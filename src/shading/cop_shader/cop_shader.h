#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>
#include <glm/glm.hpp>

#include "Ishading_algorithm.h"
#include "frame_buffer.h"
#include "shader.h"
#include "visibility_reducer.h"
#include "vertex_array.h"

namespace vat::shading {

/**
 * @brief Centre-of-pressure visibility: a triangle is visible (1.0) if its centroid survives the
 * depth test, otherwise 0.0.
 *
 * The render structure is the one of BinaryShader.
 *
 * Two passes over a depth-only framebuffer:
 *   1. depth pre-pass of the triangles (GL_LESS, depth writes on),
 *   2. one point per centroid (GL_LEQUAL, depth writes off): the fragment shader sets
 *      visible[id] = 1 for every point that passes the depth test.
 * There is no ID image and no reduction pass.
 *
 * The centroid is moved a fixed 0.005 model units along its normal, so that it lies in
 * front of its own triangle despite the small depth mismatch between a point and an
 * interpolated triangle fragment.
 *
 * Known limits:
 *   - No MSAA (num_msaa_samples must be 1).
 *   - Back faces are culled and their centroids skipped.
 */
class CoPShader : public IShadingAlgorithm {
private:
    std::unique_ptr<gl::FrameBuffer> m_frame_buffer;
    std::unique_ptr<gl::Shader> m_shader;
    std::unique_ptr<gl::Shader> m_point_shader;
    std::unique_ptr<gl::VertexArray> m_triangle_vao;
    std::unique_ptr<gl::VertexArray> m_cop_vao;
    std::unique_ptr<VisibilityReducer> m_visibility;
    const unsigned int MAX_TRIANGLES = (2u << 28) - 1; // keeps the per-triangle visibility buffer to a sane size
    unsigned int m_num_triangles = 0;
    const unsigned int NUM_PIXEL = 800;

public:
    /** @brief Distance (model units) the centroid is moved along its normal. */
    static constexpr float NORMAL_OFFSET = 0.005f;

    /**
     * @brief Stores the settings; GPU resources are created in set_vertices().
     * @param num_pixel Edge length of the square render target.
     */
    explicit CoPShader(unsigned int num_pixel);
    ~CoPShader() override;
    int set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangle_ids) override;
    std::vector<float> shade_geometry(glm::vec3 v_rel_hat, float bounding_sphere_radius,
                                      std::span<const unsigned int> num_triangles_per_mesh,
                                      std::span<const glm::mat4> model_matrices) override;
};

} // namespace vat::shading
