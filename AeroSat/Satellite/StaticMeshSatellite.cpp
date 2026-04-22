#include "StaticMeshSatellite.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cmath>
#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

StaticMeshSatellite::StaticMeshSatellite(std::string file)
    : ISatelliteShadingData(), ISatelliteManipulator(), m_total_triangles(0), m_bounding_sphere_radius(0.0f) {
    
    SPDLOG_INFO("Loading file {}", file);
    Assimp::Importer importer;
    
    // Load the scene with post-processing flags
    const aiScene* scene = importer.ReadFile(file,
        aiProcess_Triangulate |           // Ensure all faces are triangles
        aiProcess_JoinIdenticalVertices // Join identical vertices
    ); 

    if (!scene || scene->mNumMeshes == 0) {
		SPDLOG_ERROR("Failed to load model: {}", file);
        if (importer.GetErrorString()) {
			SPDLOG_ERROR("ASSIMP Error: {}", importer.GetErrorString());
        }
        return;
    }
    SPDLOG_DEBUG("Successfully loaded {} meshes", scene->mNumMeshes);
    // Extract mesh data
    for (unsigned int mesh_idx = 0; mesh_idx < scene->mNumMeshes; ++mesh_idx) {
        const aiMesh* mesh = scene->mMeshes[mesh_idx];
        SPDLOG_DEBUG("Processing mesh {} with {} faces and {} vertices", mesh_idx, mesh->mNumFaces, mesh->mNumVertices);
        unsigned int mesh_triangle_count = 0;
        // Extract triangles and normals
        for (unsigned int face_idx = 0; face_idx < mesh->mNumFaces; ++face_idx) {
            const aiFace& face = mesh->mFaces[face_idx];
            
            // Assume triangulated mesh
            if (face.mNumIndices == 3) {
                const std::uint32_t triangle_id = static_cast<std::uint32_t>(m_total_triangles + 1);
                m_triangle_ids.push_back(triangle_id);
				m_triangle_ids.push_back(triangle_id);
				m_triangle_ids.push_back(triangle_id);

                // Extract vertices
                for (unsigned int vertex_idx = 0; vertex_idx < 3; ++vertex_idx) {
                    m_vertices.push_back(mesh->mVertices[face.mIndices[vertex_idx]].x);
                    m_vertices.push_back(mesh->mVertices[face.mIndices[vertex_idx]].y);
                    m_vertices.push_back(mesh->mVertices[face.mIndices[vertex_idx]].z);
                }

                // Calculate normal and centroid from vertices
                const aiVector3D& v0 = mesh->mVertices[face.mIndices[0]];
                const aiVector3D& v1 = mesh->mVertices[face.mIndices[1]];
                const aiVector3D& v2 = mesh->mVertices[face.mIndices[2]];
                    
                aiVector3D edge1 = v1 - v0;
                aiVector3D edge2 = v2 - v0;
                aiVector3D normal = edge1 ^ edge2;
                normal.Normalize();
                    
                m_normals.push_back(normal.x);
                m_normals.push_back(normal.y);
                m_normals.push_back(normal.z);

                aiVector3D centroid = (v0 + v1 + v2) / 3.0f;
                m_centroids.push_back(centroid.x);
                m_centroids.push_back(centroid.y);
                m_centroids.push_back(centroid.z);

                // Calculate triangle area using cross product
                float area = (edge1 ^ edge2).Length() / 2.0f;
                m_areas.push_back(area);

                mesh_triangle_count++;
                m_total_triangles++;
            }
        }

        m_num_triangles_per_mesh.push_back(mesh_triangle_count);
        // Add identity model matrix for each mesh
        m_model_matrices.push_back(glm::mat4(1.0f));
    }

    // Calculate bounding sphere radius
    float max_distance = 0.0f;
    for (size_t i = 0; i < m_vertices.size(); i += 3) {
        float distance = std::sqrt(m_vertices[i] * m_vertices[i] + 
                                   m_vertices[i + 1] * m_vertices[i + 1] + 
                                   m_vertices[i + 2] * m_vertices[i + 2]);
        max_distance = std::max(max_distance, distance);
    }
    m_bounding_sphere_radius = max_distance;

	SPDLOG_INFO("Finished loading model. Total triangles: {}", m_total_triangles);
}

std::span<const float> StaticMeshSatellite::get_vertices() {
    return std::span<const float>(m_vertices.data(), m_vertices.size());
}

std::span<const std::uint32_t> StaticMeshSatellite::get_triangle_ids() {
    return std::span<const std::uint32_t>(m_triangle_ids.data(), m_triangle_ids.size());
}

std::span<const float> StaticMeshSatellite::get_normals() {
    return std::span<const float>(m_normals.data(), m_normals.size());
}

std::span<const float> StaticMeshSatellite::get_areas() {
    return std::span<const float>(m_areas.data(), m_areas.size());
}

std::span<const float> StaticMeshSatellite::get_centroids() {
    return std::span<const float>(m_centroids.data(), m_centroids.size());
}

std::span<const glm::mat4> StaticMeshSatellite::get_model_matrices() {
    return std::span<const glm::mat4>(m_model_matrices.data(), m_model_matrices.size());
}

std::span<const unsigned int> StaticMeshSatellite::get_num_triangles_per_mesh() {
    return std::span<const unsigned int>(m_num_triangles_per_mesh.data(), m_num_triangles_per_mesh.size());
}

const unsigned int StaticMeshSatellite::get_num_triangles() {
    return m_total_triangles;
}

float StaticMeshSatellite::get_bounding_sphere_radius() {
    return m_bounding_sphere_radius;
}

int StaticMeshSatellite::turn_surface(int surface_id, float angle__rad) {
    // Not implemented for static mesh
    return -1;
}
int StaticMeshSatellite::turn_surface_around_axis(const int surface_id, float angle__rad, const std::array<float, 3>& origin, const std::array<float, 3>& axis) {
    // Not implemented for static mesh
    return -1;
}
int StaticMeshSatellite::turn_surfaces() {
    // Not implemented for static mesh
    return -1;
}
