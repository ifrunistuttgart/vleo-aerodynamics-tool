#pragma once
#include <span>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

namespace vat::shading {

/**
 * @brief Interface for shading algorithms.
 *
 * Shading algorithms calculate the visibility of geometry triangles
 * based on their geometry and the direction of the incoming flow (relative velocity).
 * Implementations may use different techniques such as Z-buffering,
 * or other GPU-accelerated methods. The GPU implementations need their OpenGL context to be
 * current for every call.
 */
class IShadingAlgorithm {
public:
    virtual ~IShadingAlgorithm() = default;

    /**
     * @brief Uploads the geometry and allocates the GPU resources.
     *
     * Must succeed before shade_geometry() is called.
     *
     * @param vertices Untransformed vertex positions (see
     *                 IGeometryShadingData::get_untransformed_vertices()), 9 floats per triangle.
     * @param triangleIDs 1-based triangle ID of every vertex, each repeated three times (see
     *                    IGeometryShadingData::get_triangle_ids()); not an index buffer.
     * @return 0 on success, or a non-zero error code on failure. BinaryShader and CoPShader return
     *         -1 if there are too many triangles or the depth buffer cannot be allocated.
     */
    virtual int set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) = 0;

    /**
     * @brief Calculates the visibility of geometry triangles from a specific direction.
     *
     * This method performs the actual shading calculation, considering the geometry's
     * bounding volume and the current transformation of its individual meshes.
     *
     * @param v_rel_hat Unit vector of the velocity of the geometry relative to the atmosphere,
     *                  in the body frame.
     * @param bounding_sphere_radius Radius of the geometry's bounding sphere about the body-frame
     *                               origin [m] (see IGeometryShadingData::get_bounding_sphere_radius()).
     * @param num_triangles_per_mesh A span containing the triangle count for each mesh component.
     * @param model_matrices A span containing the transformation matrices for each mesh component.
     * @return One visibility factor per triangle, in triangle order. BinaryShader and CoPShader
     *         return exactly 0.0 or 1.0, and 0.0 for culled back faces.
     * @throws std::logic_error from BinaryShader and CoPShader if set_vertices() has not succeeded.
     */
    virtual std::vector<float> shade_geometry(glm::vec3 v_rel_hat, float bounding_sphere_radius,
                                              std::span<const unsigned int> num_triangles_per_mesh,
                                              std::span<const glm::mat4> model_matrices) = 0;
};

} // namespace vat::shading
