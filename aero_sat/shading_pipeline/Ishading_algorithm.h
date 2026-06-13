#pragma once
#include <span>
#include <cstdint>
#include <glm/glm.hpp>

/**
 * Interface for shading algorithms that calculate the visibility of satellite surfaces based on their geometry and relative velocity.
 */
class IShadingAlgorithm {
public:
	virtual ~IShadingAlgorithm() = default;

	/**
	 * Set the vertex positions and triangle IDs for the shading algorithm.
	 * @param vertices - Span containing the vertex positions, flat array with three values per vertex (x, y, z)
	 * @param triangleIDs - Span containing the triangle IDs
	 * @return 0 on success.
	 */
	virtual int set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) = 0;

	/**
	 * Calculate the shading for a set of triangles.
	 * @param v_rel_hat normalized - normalized relative velocity vector of the satellite with respect to the surrounding gas, in the satellite's body frame.
	 * @param bounding_sphere_radius - The radius of the bounding sphere that encompasses the entire satellite.
	 * @param num_triangles_per_mesh - Span containing the number of triangles in each mesh.
	 * @param model_matrices - Span containing the 4x4 transformation matrices of the satellite meshes.
	 * @return 0 on success.
	 */
	virtual std::vector<float> shade_satellite( glm::vec3 v_rel_hat, float bounding_sphere_radius, std::span<const unsigned int> num_triangles_per_mesh, std::span<const glm::mat4> model_matrices) = 0;
};
