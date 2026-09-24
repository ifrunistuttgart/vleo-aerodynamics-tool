#include "shading_pipeline.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <span>
#include <stdexcept>
#include <string>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

namespace vat::shading {

// The gl:: wrappers are this layer's own building blocks; unqualified use keeps
// the OpenGL call sites readable. TU-local, so it never leaks through a header.
using namespace gl;

namespace {

// Checked before the context exists: a 0 x 0 window would fail with a less helpful error.
unsigned int RequirePositive(unsigned int num_pixel) {
	if (num_pixel == 0) {
		throw std::invalid_argument("num_pixel must be at least 1");
	}
	return num_pixel;
}

} // namespace

ShadingPipeline::ShadingPipeline(
	IGeometryShadingData& geometry,
	ShadingAlgorithmType algorithm_type,
	unsigned int num_pixel)
	: m_context(std::make_unique<GlfwOpenGLContext>(RequirePositive(num_pixel), num_pixel, "Triangle Renderer", false)),
	  m_geometry(geometry),
	  m_num_pixel(num_pixel),
	  m_initial_radius__m(geometry.get_bounding_sphere_radius()) {
	m_context->make_current();

	// Both render targets are num_pixel square; past the driver's limit they would fail
	// deep inside the algorithm instead of here.
	GLint max_texture_size = 0;
	GLint max_renderbuffer_size = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
	glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer_size);
	const unsigned int gpu_limit = static_cast<unsigned int>(std::min(max_texture_size, max_renderbuffer_size));
	if (num_pixel > gpu_limit) {
		throw std::invalid_argument("num_pixel = " + std::to_string(num_pixel) + " is outside what this GPU "
			"supports (1 to " + std::to_string(gpu_limit) + ")");
	}
	m_algorithm = create_shading_algorithm(algorithm_type, num_pixel);

	std::span<const float> vertices = m_geometry.get_raw_vertices();
	std::span<const std::uint32_t> triangleIDs = m_geometry.get_triangle_ids();
	const int set_vertices_result = m_algorithm->set_vertices(vertices, triangleIDs);
	if (set_vertices_result != 0) {
		SPDLOG_ERROR("ShadingPipeline initialization failed in set_vertices (code={}, vertices={}, ids={})",
			set_vertices_result, vertices.size(), triangleIDs.size());
	}
}

ShadingPipeline::ShadingPipeline(
	IGeometryShadingData& geometry,
	ShadingAlgorithmType algorithm_type)
	: ShadingPipeline(geometry, algorithm_type, suggest_num_pixel(geometry, algorithm_type)) {
}

ShadingPipeline::~ShadingPipeline() {
	if (m_context) {
		m_context->make_current();
	}
	m_algorithm.reset();
}

std::vector<float> ShadingPipeline::shade( const glm::vec3& v_rel_hat) {
	m_context->make_current();

	float bsr = m_geometry.get_bounding_sphere_radius();
	// The frustum follows the current radius but num_pixel does not, so a pose that
	// reaches further out than the one at construction gets coarser pixels.
	if (!m_warned_radius_growth && bsr > 1.1f * m_initial_radius__m) {
		m_warned_radius_growth = true;
		SPDLOG_WARN("Bounding radius grew from {:.4g} m to {:.4g} m since the pipeline was built: pixels are "
			"now {:.0f} % coarser. Build the pipeline in the most extended pose, or raise num_pixel.",
			m_initial_radius__m, bsr, 100.0f * (bsr / m_initial_radius__m - 1.0f));
	}
	std::span<const glm::mat4> model_matrices = m_geometry.get_model_matrices();
	std::span<const unsigned int> num_triangles_per_mesh = m_geometry.get_num_triangles_per_mesh();
	std::vector<float> triangle_visibility = m_algorithm->shade_geometry(v_rel_hat, bsr, num_triangles_per_mesh, model_matrices);
	return triangle_visibility;
}

} // namespace vat::shading
