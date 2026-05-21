#pragma once
#include <cstdint>
#include <span>
#include <glm/glm.hpp>

/*
 * Interface for providing shading data form the satellite, that could be relevant for aero load calculations. 
 * This includes vertex positions, triangle IDs, normals, areas, centroids, and model matrices.
 */
class ISatelliteShadingData {
public:
    virtual ~ISatelliteShadingData() = default;

    /**
     * Get the vertex positions of the satellites meshes
	 * @return - Span containing the vertex positions, flat array with three values per vertex (x, y, z)
     */
    virtual std::span<const float> get_vertices() = 0;

	// TODO remove this from the compuation, since either way the correct maping of ids depends on the order of the returned data,
    // therefore the usage of triangel ids does not bring any additional benefit that just relying on the order of the returned data.
    /**
	 * Get the triangle IDs of the traingles in the satellite meshes
     * @return - Span containing the triangle IDs
     */
    virtual  std::span<const std::uint32_t> get_triangle_ids() = 0;

    /**
     * Get the normals of the triangles in the satellite meshes
     * @return - Span containing the normals, flat array with three values per normal (x, y, z)
     */
    virtual std::span<const float> get_normals() = 0;

    /**
     * Get the areas of the triangles in the satellite meshes
     * @return - Span containing the areas, flat array with one value per triangle
     */
    virtual std::span<const float> get_areas() = 0;

    /**
     * Get the centroids of the triangles in the satellite meshes
     * @return - Span containing the centroids, flat array with three values per centroid (x, y, z)
     */
    virtual std::span<const float> get_centroids() = 0;

    /**
	 * Get the 4x4 transformation matrices of the satellite meshes. resempling the translation , rotation, and scaling of the meshes.
     * @return - Span containing the model matrices
     */
    virtual std::span<const glm::mat4> get_model_matrices() = 0;

    /**
     * Get the number of triangles in each mesh
     * @return - Span containing the number of triangles per mesh
     */
    virtual std::span<const unsigned int> get_num_triangles_per_mesh() = 0;

    /**
     * Get the total number of triangles in the satellite meshes
     * @return - The total number of triangles
     */
    virtual const unsigned int get_num_triangles() = 0;

	/**
	 * Get the radius of the bounding sphere that encompasses the entire satellite in its current configuration.
	 * @return - The radius of the bounding sphere
	 */
    virtual float get_bounding_sphere_radius() = 0;
};