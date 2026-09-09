#pragma once

#include <memory>

#include "Ishading_algorithm.h"
#include "gl/frame_buffer.h"
#include "gl/shader.h"
#include "gl/visibility_reducer.h"
#include "gl/vertex_array.h"

namespace vat::shading {

class CoPShader : public IShadingAlgorithm {
private:
    std::unique_ptr<gl::FrameBuffer> m_frame_buffer;
    std::unique_ptr<gl::Shader> m_shader;
    std::unique_ptr<gl::Shader> m_point_shader;
    std::unique_ptr<gl::VertexArray> m_triangle_vao;
    std::unique_ptr<gl::VertexArray> m_cop_vao;
    const unsigned int MAX_TRIANGLES = (2u << 28) - 1; //keeps the per-triangle visibility buffer to a sane size
    size_t m_lenVertices = 0;
    unsigned int m_numTriangles = 0;
    unsigned int m_ID_texture = 0;
    std::unique_ptr<gl::VisibilityReducer> m_visibility_reducer;
    const unsigned int NUM_PIXEL = 800;
public:
    CoPShader(unsigned int num_pixel);
    ~CoPShader();
    int set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) override;
    std::vector<float> shade_geometry(glm::vec3 v_rel_hat,
                                       float bounding_sphere_radius,
                                       std::span<const unsigned int> num_triangles_per_mesh,
                                       std::span<const glm::mat4> model_matrices
                                       ) override;
};

} // namespace vat::shading
