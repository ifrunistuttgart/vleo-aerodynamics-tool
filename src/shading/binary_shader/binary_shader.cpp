#include "binary_shader.h"

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <algorithm>
#include <stdexcept>

//custom abstractions
#include "flow_camera.h"
#include "gl_helpers.h"
#include "vertex_buffer.h"
#include "vertex_buffer_layout.h"

// embedded shader headers
#include "binary_shader/shaders/id_shader.h"

namespace vat::shading {

// The gl:: wrappers are this layer's own building blocks; unqualified use keeps
// the OpenGL call sites readable. TU-local, so it never leaks through a header.
using namespace gl;

BinaryShader::BinaryShader(unsigned int num_pixel, unsigned int num_msaa_samples)
	: REQUESTED_MSAA_SAMPLES(std::max(num_msaa_samples, 1u)), NUM_PIXEL(num_pixel) {
}

BinaryShader::~BinaryShader() = default;

int BinaryShader::set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) {
	m_numTriangles = static_cast<unsigned int>(triangleIDs.size() / 3); // 3 vertices per triangle
	if (m_numTriangles > MAX_TRIANGLES) {
		SPDLOG_ERROR("Number of triangles ({}) exceeds the maximum supported ({}).", m_numTriangles, MAX_TRIANGLES);
		return -1;
	}

	// The depth buffer is a renderbuffer, so GL_MAX_SAMPLES is the limit that applies
	// (GL_MAX_DEPTH_TEXTURE_SAMPLES is for textures).
	GLint max_msaa_samples = 1;
	GLCall(glGetIntegerv(GL_MAX_SAMPLES, &max_msaa_samples));
	const auto limit = static_cast<unsigned int>(std::max(1, max_msaa_samples));
	m_num_msaa_samples = std::min(REQUESTED_MSAA_SAMPLES, limit);
	if (m_num_msaa_samples < REQUESTED_MSAA_SAMPLES) {
		SPDLOG_WARN("Requested {} MSAA samples, the driver allows {}; using {}.", REQUESTED_MSAA_SAMPLES, limit, m_num_msaa_samples);
	}

	// Depth only: visibility is marked in the SSBO, so no colour attachment is needed.
	// The sample count within the driver's limit can still be too much memory for
	// num_pixel^2 * samples * 4 bytes, so halve it until the allocation succeeds.
	for (;;) {
		SPDLOG_DEBUG("Binary shading: {}x{} px, {} sample(s), {:.0f} MB depth buffer", NUM_PIXEL, NUM_PIXEL, m_num_msaa_samples,
			static_cast<double>(NUM_PIXEL) * NUM_PIXEL * m_num_msaa_samples * 4.0 / (1024.0 * 1024.0));
		m_frame_buffer = std::make_unique<FrameBuffer>(std::span<const unsigned int>{}, NUM_PIXEL, NUM_PIXEL, m_num_msaa_samples);
		m_frame_buffer->UnBind();
		if (m_frame_buffer->is_complete()) {
			break;
		}
		m_frame_buffer.reset();
		if (m_num_msaa_samples == 1) {
			SPDLOG_ERROR("Could not create the {}x{} depth buffer.", NUM_PIXEL, NUM_PIXEL);
			return -1;
		}
		const unsigned int fewer = m_num_msaa_samples / 2;
		SPDLOG_WARN("Could not allocate the {}x{} depth buffer with {} samples; retrying with {}.",
			NUM_PIXEL, NUM_PIXEL, m_num_msaa_samples, fewer);
		m_num_msaa_samples = fewer;
	}

	// Fixed for this pipeline's own context, so set once rather than every frame.
	GLCall(glEnable(GL_MULTISAMPLE)); // no effect on a single-sample framebuffer
	GLCall(glEnable(GL_DEPTH_TEST));
	GLCall(glEnable(GL_CULL_FACE));
	GLCall(glCullFace(GL_BACK));
	GLCall(glFrontFace(GL_CCW)); // counter-clockwise is front-facing

	m_visibility = std::make_unique<VisibilityReducer>(m_numTriangles);

	m_shader = std::make_unique<Shader>(binary_glsl::vertex_shader, binary_glsl::fragment_shader, true);
	m_shader->Unbind();

	m_vao = std::make_unique<VertexArray>();
	VertexBufferLayout layout_vertices;
	layout_vertices.Push<float>(3); // vec3 position
	VertexBuffer vb(vertices.data(), static_cast<unsigned int>(sizeof(float) * vertices.size()));
	m_vao->AddBuffer(vb, layout_vertices);

	VertexBufferLayout layout_ids;
	layout_ids.Push<unsigned int>(1); // triangle ID
	VertexBuffer vb_ids(triangleIDs.data(), static_cast<unsigned int>(sizeof(std::uint32_t) * triangleIDs.size()));
	m_vao->AddBuffer(vb_ids, layout_ids);
	return 0;
}

void BinaryShader::draw_meshes(const glm::mat4& view_projection,
	std::span<const unsigned int> num_triangles_per_mesh, std::span<const glm::mat4> model_matrices) {
	unsigned int offset = 0;
	for (size_t i = 0; i < num_triangles_per_mesh.size(); ++i) {
		m_shader->setUniformMat4f("u_MVP", view_projection * model_matrices[i]);
		GLCall(glDrawArrays(GL_TRIANGLES, static_cast<GLint>(offset), static_cast<GLsizei>(num_triangles_per_mesh[i] * 3)));
		offset += num_triangles_per_mesh[i] * 3;
	}
}

std::vector<float> BinaryShader::shade_geometry(glm::vec3 v_rel_hat, float bounding_sphere_radius,
	std::span<const unsigned int> num_triangles_per_mesh, std::span<const glm::mat4> model_matrices) {
	if (!m_frame_buffer || !m_shader || !m_vao || !m_visibility) {
		throw std::logic_error("BinaryShader::shade_geometry called before a successful set_vertices");
	}
	const FlowCamera camera = make_flow_camera(v_rel_hat, bounding_sphere_radius);
	const glm::mat4 view_projection = camera.projection * camera.view;

	// Puts the depth state back on every exit path, including an exception from a draw.
	struct DepthStateGuard {
		~DepthStateGuard() {
			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LESS);
		}
	} depth_state_guard;

	m_frame_buffer->Bind();
	// The default viewport is the hidden window's size, which the OS may clamp to
	// the screen; without this, num_pixel above the screen size is silently lost.
	GLCall(glViewport(0, 0, static_cast<GLsizei>(NUM_PIXEL), static_cast<GLsizei>(NUM_PIXEL)));
	m_frame_buffer->ClearDepth();
	m_visibility->begin(0);

	m_shader->Bind();
	m_vao->Bind();

	// Pass 1: depth only. The SSBO stays untouched, because a triangle drawn before its
	// occluder would otherwise be marked.
	m_shader->SetUniform1i("u_mark", 0);
	GLCall(glDepthFunc(GL_LESS));
	GLCall(glDepthMask(GL_TRUE));
	draw_meshes(view_projection, num_triangles_per_mesh, model_matrices);

	// Pass 2: every fragment that matches the final depth of at least one sample marks
	// its triangle. Invariant positions give bit-identical depths, so no bias is needed.
	m_shader->SetUniform1i("u_mark", 1);
	GLCall(glDepthFunc(GL_LEQUAL));
	GLCall(glDepthMask(GL_FALSE));
	draw_meshes(view_projection, num_triangles_per_mesh, model_matrices);

	m_vao->Unbind();
	m_shader->Unbind();
	m_frame_buffer->UnBind();

	return m_visibility->read_back();
}

} // namespace vat::shading
