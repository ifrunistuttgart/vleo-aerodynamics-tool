#include "rotatable_mesh_geometry.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

namespace vat::geometry {

RotatableMeshGeometry::RotatableMeshGeometry(std::string file)
	: StaticMeshGeometry(file),
	  m_transformed_vertices(m_vertices.size()),
	  m_transformed_normals(m_normals.size()),
	  m_transformed_centroids(m_centroids.size()) {
}

std::span<const float> RotatableMeshGeometry::get_vertices() {
	refresh_transforms();
	return std::span<const float>(m_transformed_vertices.data(), m_transformed_vertices.size());
}

std::span<const float> RotatableMeshGeometry::get_raw_vertices() {
	return std::span<const float>(m_vertices.data(), m_vertices.size());
}

std::span<const float> RotatableMeshGeometry::get_normals() {
	refresh_transforms();
	return std::span<const float>(m_transformed_normals.data(), m_transformed_normals.size());
}

std::span<const float> RotatableMeshGeometry::get_centroids() {
	refresh_transforms();
	return std::span<const float>(m_transformed_centroids.data(), m_transformed_centroids.size());
}

float RotatableMeshGeometry::get_bounding_sphere_radius() {
	refresh_transforms();
	return m_bounding_sphere_radius;
}

void RotatableMeshGeometry::refresh_transforms() {
	if (!m_transforms_outdated) {
		return;
	}
	apply_transform(m_vertices, 9, m_transformed_vertices);
	apply_transform(m_centroids, 3, m_transformed_centroids);
	apply_normal_transform(m_normals, 3, m_transformed_normals);

	// Compare squared distances and take a single square root at the end. sqrt is
	// monotonic, so the winning vertex - and therefore the radius - is unchanged.
	float max_distance_squared = 0.0f;
	for (size_t i = 0; i < m_transformed_vertices.size(); i += 3) {
		const glm::vec3 vertex(m_transformed_vertices[i], m_transformed_vertices[i + 1], m_transformed_vertices[i + 2]);
		max_distance_squared = std::max(max_distance_squared, glm::dot(vertex, vertex));
	}
	m_bounding_sphere_radius = std::sqrt(max_distance_squared);

	m_transforms_outdated = false;
}

int RotatableMeshGeometry::turn_mesh_around_axis(const int mesh_id, float angle__rad, const std::array<float, 3>& origin, const std::array<float, 3>& axis) {
	if (mesh_id < 0 || mesh_id >= static_cast<int>(m_model_matrices.size())) {
		SPDLOG_ERROR("turn_mesh_around_axis invalid mesh_id={} (num_meshes={})", mesh_id, m_model_matrices.size());
		return -1;
	}
	// Create rotation matrix
	glm::mat4 translation_to_origin = glm::translate(glm::mat4(1.0f), glm::vec3(-origin[0], -origin[1], -origin[2]));
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle__rad, glm::vec3(axis[0], axis[1], axis[2])); //TODO: consider normalizing axis vector
	glm::mat4 translation_back = glm::translate(glm::mat4(1.0f), glm::vec3(origin[0], origin[1], origin[2]));
	glm::mat4 transform = translation_back * rotation * translation_to_origin;

	// Apply transformation to the specified mesh.s vertices
	m_model_matrices[mesh_id] = transform;
	m_transforms_outdated = true;
	return 0; // Success
}

void RotatableMeshGeometry::apply_transform(std::span<const float> coordinates, int num_entries_per_triangle, std::vector<float>& target) const {
	size_t offset = 0;
	for (size_t mesh_id = 0; mesh_id < m_model_matrices.size(); ++mesh_id) {
		const glm::mat4& transform = m_model_matrices[mesh_id];
		const size_t end = offset + static_cast<size_t>(m_num_triangles_per_mesh[mesh_id]) * num_entries_per_triangle;

		for (size_t i = offset; i < end; i += 3) {
			glm::vec4 vertex(coordinates[i], coordinates[i + 1], coordinates[i + 2], 1.0f);
			glm::vec4 transformed_vertex = transform * vertex;
			target[i] = transformed_vertex.x;
			target[i + 1] = transformed_vertex.y;
			target[i + 2] = transformed_vertex.z;
		}

		offset = end; // Move to the next mesh's vertices
	}
}

void RotatableMeshGeometry::apply_normal_transform(std::span<const float> normals, int num_entries_per_triangle, std::vector<float>& target) const {
	size_t offset = 0;
	for (size_t mesh_id = 0; mesh_id < m_model_matrices.size(); ++mesh_id) {
		glm::mat3 normal_transform = glm::transpose(glm::inverse(glm::mat3(m_model_matrices[mesh_id])));
		const size_t end = offset + static_cast<size_t>(m_num_triangles_per_mesh[mesh_id]) * num_entries_per_triangle;

		for (size_t i = offset; i < end; i += 3) {
			glm::vec3 normal(normals[i], normals[i + 1], normals[i + 2]);
			glm::vec3 transformed_normal = glm::normalize(normal_transform * normal);
			target[i] = transformed_normal.x;
			target[i + 1] = transformed_normal.y;
			target[i + 2] = transformed_normal.z;
		}

		offset = end;
	}
}

} // namespace vat::geometry
