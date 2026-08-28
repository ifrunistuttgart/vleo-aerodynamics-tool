#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <algorithm>
#include <cmath>
#include <vector>

#include "static_mesh_geometry.h"

namespace vat {
namespace {

/*
 * Warns when a model separates its parts with `g` groups instead of `o` objects. A file
 * whose parts are groups of box faces loads as one mesh per face, so every mesh_id then
 * addresses a fragment and rotating one tears a face off a part.
 *
 * Assimp keeps a group only as an empty node -- no meshes, no children -- which is what
 * this counts. Files using `o` alone, and formats without groups, have none.
 */
void WarnIfFileUsesGroups(const aiScene* scene, const std::string& file) {
    const aiNode* root = scene->mRootNode;
    unsigned int num_group_markers = 0;
    for (unsigned int i = 0; i < root->mNumChildren; ++i) {
        const aiNode* child = root->mChildren[i];
        if (child->mNumMeshes == 0 && child->mNumChildren == 0) {
            ++num_group_markers;
        }
    }

    if (num_group_markers == 0) {
        return;
    }

    SPDLOG_WARN("{} appears to separate its parts with groups: found {} group(s) but {} "
        "mesh(es). Do not use groups in your .obj export -- use the 'o' identifier to "
        "separate meshes. Each mesh is what turn_mesh_around_axis() rotates, so with "
        "groups every mesh_id addresses only a fragment of a part.",
        file, num_group_markers, scene->mNumMeshes);
}

} // namespace

StaticMeshGeometry::StaticMeshGeometry(std::string file)
    : IGeometryShadingData(), IGeometryManipulator(), m_total_triangles(0), m_bounding_sphere_radius(0.0f) {
    
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
    WarnIfFileUsesGroups(scene, file);

    // Extract mesh data. One entry of scene->mMeshes is one mesh of the geometry, i.e.
    // one `o` object of an .obj, and is the unit turn_mesh_around_axis() rotates.
    for (unsigned int mesh_idx = 0; mesh_idx < scene->mNumMeshes; ++mesh_idx) {
        const aiMesh* mesh = scene->mMeshes[mesh_idx];
        SPDLOG_DEBUG("Processing mesh {} (\"{}\") with {} faces and {} vertices",
            mesh_idx, mesh->mName.C_Str(), mesh->mNumFaces, mesh->mNumVertices);
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
        // Keep the author's name for the mesh so the visualization can label it. Not
        // every format carries names, in which case the index has to do.
        const std::string mesh_name = mesh->mName.length > 0
            ? std::string(mesh->mName.C_Str())
            : "Mesh " + std::to_string(mesh_idx);
        m_mesh_names.push_back(mesh_name);
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

std::span<const float> StaticMeshGeometry::get_vertices() {
    return std::span<const float>(m_vertices.data(), m_vertices.size());
}

std::span<const std::uint32_t> StaticMeshGeometry::get_triangle_ids() {
    return std::span<const std::uint32_t>(m_triangle_ids.data(), m_triangle_ids.size());
}

std::span<const float> StaticMeshGeometry::get_normals() {
    return std::span<const float>(m_normals.data(), m_normals.size());
}

std::span<const float> StaticMeshGeometry::get_areas() {
    return std::span<const float>(m_areas.data(), m_areas.size());
}

std::span<const float> StaticMeshGeometry::get_centroids() {
    return std::span<const float>(m_centroids.data(), m_centroids.size());
}

std::span<const glm::mat4> StaticMeshGeometry::get_model_matrices() {
    return std::span<const glm::mat4>(m_model_matrices.data(), m_model_matrices.size());
}

std::span<const std::string> StaticMeshGeometry::get_mesh_names() {
    return std::span<const std::string>(m_mesh_names.data(), m_mesh_names.size());
}

std::span<const unsigned int> StaticMeshGeometry::get_num_triangles_per_mesh() {
    return std::span<const unsigned int>(m_num_triangles_per_mesh.data(), m_num_triangles_per_mesh.size());
}

const unsigned int StaticMeshGeometry::get_num_triangles() {
    return m_total_triangles;
}

float StaticMeshGeometry::get_bounding_sphere_radius() {
    return m_bounding_sphere_radius;
}

int StaticMeshGeometry::turn_mesh(int mesh_id, float angle__rad) {
    // Not implemented for static mesh
    return -1;
}
int StaticMeshGeometry::turn_mesh_around_axis(const int mesh_id, float angle__rad, const std::array<float, 3>& origin, const std::array<float, 3>& axis) {
    // Not implemented for static mesh
    return -1;
}
int StaticMeshGeometry::turn_meshes() {
    // Not implemented for static mesh
    return -1;
}

} // namespace vat
