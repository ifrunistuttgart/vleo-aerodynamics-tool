#pragma once

#include <memory>

#include "Ishading_algorithm.h"
#include "frame_buffer.h"
#include "shader.h"
#include "compute_shader.h"
#include "vertex_array.h"
#include "texture_2d.h"

class CoPShader : public IShadingAlgorithm {
private:
    std::unique_ptr<FrameBuffer> m_frame_buffer;
    std::unique_ptr<Shader> m_shader;
    std::unique_ptr<Shader> m_point_shader;
    std::unique_ptr<ComputeShader> m_compute_shader;
    std::unique_ptr<VertexArray> m_triangle_vao;
    std::unique_ptr<VertexArray> m_cop_vao;
    std::unique_ptr<Texture2D> m_texture;
    const unsigned int MAX_TRIANGLES = (2u << 28) - 1; //limit histogrambuffer size to about 1GB
    size_t m_lenVertices = 0;
    unsigned int m_numTriangles = 0;
    unsigned int m_histogramBuffer = 0;
    const unsigned int NUM_PIXEL = 800;
public:
    CoPShader(unsigned int num_pixel);
    ~CoPShader();
    int set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) override;
    std::vector<float> shade_satellite(glm::vec3 v_rel_hat,
                                       float bounding_sphere_radius,
                                       std::span<const unsigned int> num_triangles_per_mesh,
                                       std::span<const glm::mat4> model_matrices
                                       ) override;
};
