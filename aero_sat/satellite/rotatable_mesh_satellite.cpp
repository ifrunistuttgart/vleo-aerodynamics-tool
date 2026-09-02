#include "rotatable_mesh_satellite.h"
#include <array>
#include <glm/gtc/matrix_transform.hpp>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

RotatableMeshSatellite::RotatableMeshSatellite(std::string file)
	: StaticMeshSatellite(file) {
}

void RotatableMeshSatellite::refresh_transformed_data() {
	m_transformed_vertices = apply_transform(m_vertices, 9);
	m_transformed_normals = apply_normal_transform(m_normals, 3);
	m_transformed_centroids = apply_transform(m_centroids, 3);
	float max_distance = 0.0f;
	for (size_t i = 0; i < m_transformed_vertices.size(); i += 3) {
		float distance = std::sqrt(m_transformed_vertices[i] * m_transformed_vertices[i] +
			m_transformed_vertices[i + 1] * m_transformed_vertices[i + 1] +
			m_transformed_vertices[i + 2] * m_transformed_vertices[i + 2]);
		max_distance = std::max(max_distance, distance);
	}
	m_bounding_sphere_radius = max_distance;
	m_transformed_data_current = true;
}

std::span<const float> RotatableMeshSatellite::get_vertices() {
	if (!m_transformed_data_current) {
		refresh_transformed_data();
	}
	return std::span<const float>(m_transformed_vertices.data(), m_transformed_vertices.size());
}

std::span<const float> RotatableMeshSatellite::get_raw_vertices() {
	return std::span<const float>(m_vertices.data(), m_vertices.size());
}

std::span<const float> RotatableMeshSatellite::get_normals() {
	if (!m_transformed_data_current) {
		refresh_transformed_data();
	}
	return std::span<const float>(m_transformed_normals.data(), m_transformed_normals.size());
}

std::span<const float> RotatableMeshSatellite::get_centroids() {
	if (!m_transformed_data_current) {
		refresh_transformed_data();
	}
	return std::span<const float>(m_transformed_centroids.data(), m_transformed_centroids.size());
}

float RotatableMeshSatellite::get_bounding_sphere_radius() {
	if (!m_transformed_data_current) {
		refresh_transformed_data();
	}
	return m_bounding_sphere_radius;
}

//TODO meshid statt surface id
int RotatableMeshSatellite::turn_surface_around_axis(const int surface_id, float angle__rad, const std::array<float, 3>& origin, const std::array<float, 3>& axis) {
	if (surface_id < 0 || surface_id >= static_cast<int>(m_model_matrices.size())) {
		SPDLOG_ERROR("turn_surface_around_axis invalid surface_id={} (num_surfaces={})", surface_id, m_model_matrices.size());
		return -1;
	}
	// Create rotation matrix
	glm::mat4 translation_to_origin = glm::translate(glm::mat4(1.0f), glm::vec3(-origin[0], -origin[1], -origin[2]));
	glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle__rad, glm::vec3(axis[0], axis[1], axis[2])); //TODO: consider normalizing axis vector
	glm::mat4 translation_back = glm::translate(glm::mat4(1.0f), glm::vec3(origin[0], origin[1], origin[2]));
	glm::mat4 transform = translation_back * rotation * translation_to_origin;

	// Apply transformation to the specified surface's vertices
	m_model_matrices[surface_id] = transform;
	m_transformed_data_current = false;
	return 0; // Success
}

std::vector<float> RotatableMeshSatellite::apply_transform(std::span<float> coordinates, int num_entries_per_triangle) {
	std::vector<float> transformed(coordinates.begin(), coordinates.end());

	int offset = 0;
	for (int mesh_id = 0; mesh_id < m_model_matrices.size(); ++mesh_id) {
		glm::mat4 transform = m_model_matrices[mesh_id];

		for (size_t i = offset; i < (offset + m_num_triangles_per_mesh[mesh_id] * num_entries_per_triangle); i += 3) {
			glm::vec4 vertex(coordinates[i], coordinates[i + 1], coordinates[i + 2], 1.0f);
			glm::vec4 transformed_vertex = transform * vertex;
			transformed[i] = transformed_vertex.x;
			transformed[i + 1] = transformed_vertex.y;
			transformed[i + 2] = transformed_vertex.z;
		}
		
		offset += m_num_triangles_per_mesh[mesh_id] * num_entries_per_triangle; // Move to the next mesh's vertices
	}
	return transformed;
}

std::vector<float> RotatableMeshSatellite::apply_normal_transform(std::span<float> normals, int num_entries_per_triangle) {
	std::vector<float> transformed(normals.begin(), normals.end());

	int offset = 0;
	for (int mesh_id = 0; mesh_id < m_model_matrices.size(); ++mesh_id) {
		glm::mat3 normal_transform = glm::transpose(glm::inverse(glm::mat3(m_model_matrices[mesh_id])));

		for (size_t i = offset; i < (offset + m_num_triangles_per_mesh[mesh_id] * num_entries_per_triangle); i += 3) {
			glm::vec3 normal(normals[i], normals[i + 1], normals[i + 2]);
			glm::vec3 transformed_normal = glm::normalize(normal_transform * normal);
			transformed[i] = transformed_normal.x;
			transformed[i + 1] = transformed_normal.y;
			transformed[i + 2] = transformed_normal.z;
		}

		offset += m_num_triangles_per_mesh[mesh_id] * num_entries_per_triangle;
	}
	return transformed;
}