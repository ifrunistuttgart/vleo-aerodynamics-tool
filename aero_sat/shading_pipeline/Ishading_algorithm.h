#pragma once
#include <span>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

/**
 * Interface for shading algorithms.
 *
 * Shading algorithms calculate the visibility of satellite surface elements 
 * based on their geometry and the direction of the incoming flow (relative velocity).
 * Implementations may use different techniques such as Z-buffering,
 * or other GPU-accelerated methods.
 */
class IShadingAlgorithm {
public:
	virtual ~IShadingAlgorithm() = default;

	/**
	 * Sets the geometric data for the shading algorithm.
	 *
	 * This method initializes the algorithm with the vertex positions and triangle
	 * definitions of the satellite model.
	 *
	 * @param vertices A span containing the vertex positions (x, y, z triplets).
	 * @param triangleIDs A span containing the triangle IDs (indices).
	 * @return 0 on success, or a non-zero error code on failure.
	 */
	virtual int set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) = 0;

	/**
	 * Calculates the visibility of satellite triangles from a specific direction.
	 *
	 * This method performs the actual shading calculation, considering the satellite's
	 * bounding volume and the current transformation of its individual meshes.
	 *
	 * @param v_rel_hat The normalized relative velocity vector in the satellite's body frame.
	 * @param bounding_sphere_radius The radius of the satellite's bounding sphere.
	 * @param num_triangles_per_mesh A span containing the triangle count for each mesh component.
	 * @param model_matrices A span containing the transformation matrices for each mesh component.
	 * @return A vector of visibility factors (one per triangle).
	 */
	virtual std::vector<float> shade_satellite(glm::vec3 v_rel_hat,
											   float bounding_sphere_radius,
											   std::span<const unsigned int> num_triangles_per_mesh,
											   std::span<const glm::mat4> model_matrices
											   ) = 0;
};
