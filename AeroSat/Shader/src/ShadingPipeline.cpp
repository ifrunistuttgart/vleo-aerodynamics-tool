#include "src/ShadingPipeline.h"
#include <glm/glm.hpp>

ShadingPipeline::ShadingPipeline(
    ISatelliteShadingData& satellite,
    ShadingAlgorithmType algorithm_type,
    unsigned int num_pixel)
    : m_context(std::make_unique<GlfwOpenGLContext>(800, 800, "Triangle Renderer", false)),
      m_algorithm(create_shading_algorithm(algorithm_type, num_pixel)),
      m_satellite(satellite) {
    m_context->make_current();

    float* vertices = m_satellite.get_vertices();
    size_t lenVertices = m_satellite.get_num_vertices();
    unsigned int* triangleIDs = m_satellite.get_triangle_ids();
    size_t lenTriangleIDs = m_satellite.get_num_triangle_ids();
    m_algorithm->set_vertices(vertices, lenVertices, triangleIDs, lenTriangleIDs);
}

ShadingPipeline::~ShadingPipeline() {
    if (m_context) {
        m_context->make_current();
    }
    m_algorithm.reset();
}

int ShadingPipeline::shade(float* triangle_visibility, const Eigen::Vector3f& v_rel_hat) {
    m_context->make_current();

    float bsr = m_satellite.get_bounding_sphere_radius();
    glm::vec3 v_rel_hat_glm(v_rel_hat.x(), v_rel_hat.y(), v_rel_hat.z());
    return m_algorithm->shade_satellite(triangle_visibility, v_rel_hat_glm, bsr);
}