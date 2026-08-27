#include "shading_pipeline.h"
#include <glm/glm.hpp>
#include <span>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

namespace vat {

using namespace gl;


ShadingPipeline::ShadingPipeline(
	IGeometryShadingData& geometry,
	ShadingAlgorithmType algorithm_type,
	unsigned int num_pixel)
	: m_context(std::make_unique<GlfwOpenGLContext>(num_pixel, num_pixel, "Triangle Renderer", false)),
	  m_algorithm(create_shading_algorithm(algorithm_type, num_pixel)),
	  m_geometry(geometry) {
	m_context->make_current();

	std::span<const float> vertices = m_geometry.get_vertices();
	std::span<const std::uint32_t> triangleIDs = m_geometry.get_triangle_ids();
	const int set_vertices_result = m_algorithm->set_vertices(vertices, triangleIDs);
	if (set_vertices_result != 0) {
		SPDLOG_ERROR("ShadingPipeline initialization failed in set_vertices (code={}, vertices={}, ids={})",
			set_vertices_result, vertices.size(), triangleIDs.size());
	}
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
	std::span<const glm::mat4> model_matrices = m_geometry.get_model_matrices();
	std::span<const unsigned int> num_triangles_per_mesh = m_geometry.get_num_triangles_per_mesh();
	std::vector<float> triangle_visibility = m_algorithm->shade_geometry(v_rel_hat, bsr, num_triangles_per_mesh, model_matrices);
	return triangle_visibility;
}

} // namespace vat
