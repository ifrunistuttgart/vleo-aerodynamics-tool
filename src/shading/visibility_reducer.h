#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "compute_shader.h"
#include "shader_storage_buffer.h"

namespace vat::shading {

/**
 * @brief Per-triangle visibility flags on the GPU, read back as one visibility value per triangle.
 *
 * The main path, used by BinaryShader and CoPShader: begin() clears the flag buffer and binds it
 * as an SSBO, the shaders mark every visible triangle themselves (`visible[id] = 1u`), and
 * read_back() returns the flags. Only one flag per triangle is read back.
 *
 * reduce() is the older path for a rendered triangle-ID image: a compute shader scans the ID
 * texture and marks every ID it finds, i.e. a triangle is visible if at least one pixel
 * carries its ID. It currently has no callers.
 *
 * A context must be current for every call.
 */
class VisibilityReducer {
public:
    /**
     * @brief Compiles the reduction shader and allocates the flag buffer.
     *
     * @param num_triangles Total number of triangles; triangle IDs are 1-based, so the
     *                      flag buffer holds one extra slot for the background (ID 0).
     */
    explicit VisibilityReducer(unsigned int num_triangles);
    ~VisibilityReducer();

    VisibilityReducer(const VisibilityReducer&) = delete;
    VisibilityReducer& operator=(const VisibilityReducer&) = delete;

    /**
     * @brief Reduces a rendered triangle-ID image to visibility flags (currently unused).
     *
     * Rebinds the flag buffer to SSBO binding 0.
     *
     * @param id_texture R32UI texture holding the rendered triangle IDs.
     * @param num_pixel Edge length of the square ID texture.
     * @return 1.0 for every visible triangle, 0.0 otherwise, in triangle order.
     */
    std::vector<float> reduce(unsigned int id_texture, unsigned int num_pixel);

    /**
     * @brief Clears the visibility flags and binds them as SSBO `binding`, for shaders that
     * mark triangles themselves (`visible[id] = 1u`) instead of rendering an ID image.
     *
     * Follow with read_back() once those shaders have run.
     *
     * @param binding SSBO binding point the shaders declare.
     */
    void begin(unsigned int binding = 0);

    /**
     * @brief Waits for the shader writes and reads the flags back.
     * @return 1.0 for every marked triangle, 0.0 otherwise, in triangle order.
     */
    std::vector<float> read_back();

private:
    std::unique_ptr<gl::ComputeShader> m_shader;
    gl::ShaderStorageBuffer m_visibility_buffer; // one flag per triangle, plus slot 0 for the background
    unsigned int m_num_triangles = 0;
    std::vector<std::uint32_t> m_flags; // reused staging buffer for the readback
};

} // namespace vat::shading
