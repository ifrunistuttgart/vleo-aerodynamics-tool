#include "rotatable_mesh_satellite.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

RotatableMeshSatellite::RotatableMeshSatellite(std::string file)
	: StaticMeshSatellite(file),
	  m_transformed_vertices(m_vertices.size()),
	  m_transformed_normals(m_normals.size()),
	  m_transformed_centroids(m_centroids.size()) {
}

std::span<const float> RotatableMeshSatellite::get_vertices() {
	refresh_transforms();
	return m_transformed_vertices;
}

std::span<const float> RotatableMeshSatellite::get_normals() {
	refresh_transforms();
	return m_transformed_normals;
}

std::span<const float> RotatableMeshSatellite::get_centroids() {
	refresh_transforms();
	return m_transformed_centroids;
}

float RotatableMeshSatellite::get_bounding_sphere_radius() {
	refresh_transforms();
	return m_bounding_sphere_radius;
}

void RotatableMeshSatellite::refresh_transforms() {
	if (!m_transforms_outdated) {
		return;
	}
	transform_positions(m_vertices, 9, m_transformed_vertices);
	transform_positions(m_centroids, 3, m_transformed_centroids);
	transform_directions(m_normals, m_transformed_normals);

	float max_distance_squared = 0.0f;
	for (size_t i = 0; i < m_transformed_vertices.size(); i += 3) {
		const glm::vec3 vertex(m_transformed_vertices[i], m_transformed_vertices[i + 1], m_transformed_vertices[i + 2]);
		max_distance_squared = std::max(max_distance_squared, glm::dot(vertex, vertex));
	}
	m_bounding_sphere_radius = std::sqrt(max_distance_squared);

	m_transforms_outdated = false;
}

//TODO meshid statt surface id
int RotatableMeshSatellite::turn_surface_around_axis(const int surface_id, float angle__rad, const std::array<float, 3>& origin, const std::array<float, 3>& axis) {
	if (surface_id < 0 || surface_id >= static_cast<int>(m_model_matrices.size())) {
		SPDLOG_ERROR("turn_surface_around_axis invalid surface_id={} (num_surfaces={})", surface_id, m_model_matrices.size());
		return -1;
	}
	// Create rotation matrix (glm::rotate normalizes the axis internally)
	glm::mat4 translation_to_origin = glm::translate(glm::mat4(1.0f), glm::vec3(-origin[0], -origin[1], -origin[2]));
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle__rad, glm::vec3(axis[0], axis[1], axis[2]));
	glm::mat4 translation_back = glm::translate(glm::mat4(1.0f), glm::vec3(origin[0], origin[1], origin[2]));
	glm::mat4 transform = translation_back * rotation * translation_to_origin;

	// Applied to the pristine geometry, so this is absolute, not incremental.
	m_model_matrices[surface_id] = transform;
	m_transforms_outdated = true;
	return 0; // Success
}

void RotatableMeshSatellite::transform_positions(std::span<const float> source, int floats_per_triangle, std::vector<float>& target) const {
	size_t offset = 0;
	for (size_t mesh_id = 0; mesh_id < m_model_matrices.size(); ++mesh_id) {
		const glm::mat4& model = m_model_matrices[mesh_id];
		const size_t end = offset + static_cast<size_t>(m_num_triangles_per_mesh[mesh_id]) * floats_per_triangle;

		for (size_t i = offset; i < end; i += 3) {
			const glm::vec3 position(model * glm::vec4(source[i], source[i + 1], source[i + 2], 1.0f));
			target[i] = position.x;
			target[i + 1] = position.y;
			target[i + 2] = position.z;
		}
		offset = end;
	}
}

void RotatableMeshSatellite::transform_directions(std::span<const float> source, std::vector<float>& target) const {
	size_t offset = 0;
	for (size_t mesh_id = 0; mesh_id < m_model_matrices.size(); ++mesh_id) {
		// Only the linear block: a direction must not pick up the translation column,
		// which is (I - R) * origin and therefore non-zero whenever the rotation axis
		// misses the body origin. The block is a pure rotation, so it is already the
		// correct normal matrix and preserves unit length; introducing scaling here
		// would require the inverse transpose instead.
		const glm::mat3 rotation(m_model_matrices[mesh_id]);
		const size_t end = offset + static_cast<size_t>(m_num_triangles_per_mesh[mesh_id]) * 3;

		for (size_t i = offset; i < end; i += 3) {
			const glm::vec3 direction = rotation * glm::vec3(source[i], source[i + 1], source[i + 2]);
			target[i] = direction.x;
			target[i + 1] = direction.y;
			target[i + 2] = direction.z;
		}
		offset = end;
	}
}
