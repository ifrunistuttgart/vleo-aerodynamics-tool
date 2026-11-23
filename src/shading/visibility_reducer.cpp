#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include <span>

#include "gl_helpers.h"
#include "memory_barrier.h"
#include "visibility_reducer.h"

namespace vat::shading {

// The gl:: wrappers are this layer's own building blocks; unqualified use keeps
// the OpenGL call sites readable. TU-local, so it never leaks through a header.
using namespace gl;

namespace {

constexpr unsigned int LOCAL_SIZE = 16;

// The ID image is read with texelFetch on a usampler2D rather than imageLoad on a
// uimage2D: imageLoad from an integer image returns zeros on some AMD and Intel
// drivers, whereas an ordinary texture fetch is reliable everywhere.
inline constexpr const char* VISIBILITY_REDUCTION_SHADER = R"GLSL(
#version 430

layout(local_size_x = 16, local_size_y = 16) in;

uniform usampler2D u_triangle_ids;

layout(std430, binding = 0) buffer VisibilityBuffer
{
    uint visible[];
};

void main()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 size = textureSize(u_triangle_ids, 0);
    if (coord.x >= size.x || coord.y >= size.y)
    {
        return;
    }

    uint id = texelFetch(u_triangle_ids, coord, 0).r;
    if (id > 0u && id < uint(visible.length()))
    {
        // Every writer stores the same value, so the race between them is benign
        // and no atomic is needed.
        visible[id] = 1u;
    }
}
)GLSL";

} // namespace

VisibilityReducer::VisibilityReducer(unsigned int num_triangles)
	: m_shader(std::make_unique<gl::ComputeShader>(VISIBILITY_REDUCTION_SHADER)),
	  m_visibility_buffer((static_cast<size_t>(num_triangles) + 1) * sizeof(std::uint32_t), BufferUsage::DynamicRead),
	  m_num_triangles(num_triangles),
	  m_flags(static_cast<size_t>(num_triangles) + 1, 0) {
	m_shader->Unbind();

	SPDLOG_DEBUG("VisibilityReducer ready for {} triangles ({} bytes read back per shade)",
		num_triangles, m_flags.size() * sizeof(std::uint32_t));
}

VisibilityReducer::~VisibilityReducer() = default;

std::vector<float> VisibilityReducer::reduce(unsigned int id_texture, unsigned int num_pixel) {
	begin(0);

	m_shader->Bind();
	m_shader->SetUniform1i("u_triangle_ids", 0);
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, id_texture));

	const GLuint num_groups = (num_pixel + LOCAL_SIZE - 1) / LOCAL_SIZE;
	m_shader->dispatch(num_groups, num_groups);

	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	m_shader->Unbind();

	return read_back();
}

void VisibilityReducer::begin(unsigned int binding) {
	m_visibility_buffer.clear();
	m_visibility_buffer.bind_base(binding);
}

std::vector<float> VisibilityReducer::read_back() {
	memory_barrier(Barrier::ShaderStorage | Barrier::BufferUpdate);
	m_visibility_buffer.read_back(std::span<std::uint32_t>(m_flags));

	// Triangle IDs are 1-based, so slot 0 is the background and is skipped.
	std::vector<float> triangle_visibility(m_num_triangles, 0.0f);
	for (unsigned int i = 0; i < m_num_triangles; ++i) {
		triangle_visibility[i] = m_flags[static_cast<size_t>(i) + 1] != 0 ? 1.0f : 0.0f;
	}
	return triangle_visibility;
}

} // namespace vat::shading
