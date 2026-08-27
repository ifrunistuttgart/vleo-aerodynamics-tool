#pragma once
#include <cstdint>
#include <span>
#include <glm/glm.hpp>

namespace vat {

/**
 * Interface for providing geometric and shading-related data of the satellite.
 *
 * This data is used for aerodynamic load calculations and visibility analysis.
 * It includes vertex positions, triangle definitions, normals, and physical properties
 * like areas and centroids of the satellite's surface elements.
 */
class ISatelliteShadingData {
public:
    virtual ~ISatelliteShadingData() = default;

    /**
     * Retrieves the vertex positions of the satellite's meshes.
     *
     * @return A span containing vertex positions (x, y, z triplets).
     */
    virtual std::span<const float> get_vertices() = 0;

    /**
     * Retrieves the triangle IDs for the satellite's meshes.
     *
     * Note: This mapping is essential for identifying which vertices form a triangle,
     * although it's recommended to rely on consistent data ordering where possible.
     *
     * @return A span containing the triangle IDs (indices into the vertex array).
     */
    virtual  std::span<const std::uint32_t> get_triangle_ids() = 0;

    /**
     * Retrieves the surface normals of the triangles in the satellite meshes.
     *
     * @return A span containing the normal vectors (x, y, z triplets).
     */
    virtual std::span<const float> get_normals() = 0;

    /**
     * Retrieves the surface areas of the triangles in the satellite meshes.
     *
     * @return A span containing the area of each triangle.
     */
    virtual std::span<const float> get_areas() = 0;

    /**
     * Retrieves the centroids of the triangles in the satellite meshes.
     *
     * @return A span containing the centroid positions (x, y, z triplets).
     */
    virtual std::span<const float> get_centroids() = 0;

    /**
     * Retrieves the 4x4 transformation (model) matrices for each satellite mesh.
     *
     * These matrices define the current position, rotation, and scale of each component.
     *
     * @return A span containing the model matrices.
     */
    virtual std::span<const glm::mat4> get_model_matrices() = 0;

    /**
     * Retrieves the number of triangles in each individual mesh of the satellite.
     *
     * @return A span containing the triangle count per mesh.
     */
    virtual std::span<const unsigned int> get_num_triangles_per_mesh() = 0;

    /**
     * Retrieves the total number of triangles across all meshes of the satellite.
     *
     * @return The total triangle count.
     */
    virtual const unsigned int get_num_triangles() = 0;

    /**
     * Retrieves the radius of a bounding sphere that encompasses the entire satellite
     * in its current configuration.
     *
     * @return The bounding sphere radius.
     */
    virtual float get_bounding_sphere_radius() = 0;
};

} // namespace vat
