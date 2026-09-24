#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <cmath>
#include <stdexcept>

#include "static_mesh_geometry.h"

namespace vat::geometry {

StaticMeshGeometry::StaticMeshGeometry(std::string file)
    : StaticMeshGeometry(load_mesh_data(file)) {
}

StaticMeshGeometry::StaticMeshGeometry(std::vector<MeshData> meshes)
    : IGeometryShadingData(), IGeometryManipulator(),
      m_mesh_data(std::move(meshes)), m_total_triangles(0), m_bounding_sphere_radius(0.0f) {

    for (std::size_t mesh_idx = 0; mesh_idx < m_mesh_data.size(); ++mesh_idx) {
        MeshData& mesh = m_mesh_data[mesh_idx];
        const std::size_t num_vertices = mesh.positions.size() / 3;
        if (mesh.positions.size() % 3 != 0 || mesh.indices.size() % 3 != 0) {
            throw std::invalid_argument("mesh " + std::to_string(mesh_idx)
                + ": positions and indices must both come in triples");
        }
        for (const std::uint32_t index : mesh.indices) {
            if (index >= num_vertices) {
                throw std::invalid_argument("mesh " + std::to_string(mesh_idx)
                    + " references vertex " + std::to_string(index)
                    + ", but has only " + std::to_string(num_vertices));
            }
        }

        const unsigned int mesh_triangle_count = static_cast<unsigned int>(mesh.indices.size() / 3);
        for (unsigned int t = 0; t < mesh_triangle_count; ++t) {
            const std::uint32_t triangle_id = static_cast<std::uint32_t>(m_total_triangles + 1);
            m_triangle_ids.push_back(triangle_id);
            m_triangle_ids.push_back(triangle_id);
            m_triangle_ids.push_back(triangle_id);

            const float* v[3];
            for (unsigned int corner = 0; corner < 3; ++corner) {
                v[corner] = &mesh.positions[3 * mesh.indices[3 * t + corner]];
                m_vertices.push_back(v[corner][0]);
                m_vertices.push_back(v[corner][1]);
                m_vertices.push_back(v[corner][2]);
            }

            // Written out component by component in exactly the operation order of the
            // aiVector3D arithmetic this replaced (cross product, then scale by 1/|n|;
            // centroid as sum times 1/3), so results stay bit-identical to before.
            const float e1[3] = {v[1][0] - v[0][0], v[1][1] - v[0][1], v[1][2] - v[0][2]};
            const float e2[3] = {v[2][0] - v[0][0], v[2][1] - v[0][1], v[2][2] - v[0][2]};
            const float cross[3] = {
                e1[1] * e2[2] - e1[2] * e2[1],
                e1[2] * e2[0] - e1[0] * e2[2],
                e1[0] * e2[1] - e1[1] * e2[0],
            };
            const float cross_length = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);

            float normal[3] = {cross[0], cross[1], cross[2]};
            if (cross_length != 0.0f) {
                const float inv_length = 1.0f / cross_length;
                normal[0] *= inv_length;
                normal[1] *= inv_length;
                normal[2] *= inv_length;
            }
            m_normals.push_back(normal[0]);
            m_normals.push_back(normal[1]);
            m_normals.push_back(normal[2]);

            const float one_third = 1.0f / 3.0f;
            m_centroids.push_back((v[0][0] + v[1][0] + v[2][0]) * one_third);
            m_centroids.push_back((v[0][1] + v[1][1] + v[2][1]) * one_third);
            m_centroids.push_back((v[0][2] + v[1][2] + v[2][2]) * one_third);

            m_areas.push_back(cross_length / 2.0f);

            m_total_triangles++;
        }

        m_num_triangles_per_mesh.push_back(mesh_triangle_count);
        // Not every format carries mesh names, in which case the index has to do. Stored
        // back into the MeshData too, so an exported file keeps the same labels.
        if (mesh.name.empty()) {
            mesh.name = "Mesh " + std::to_string(mesh_idx);
        }
        m_mesh_names.push_back(mesh.name);
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

std::span<const MeshData> StaticMeshGeometry::get_mesh_data() const {
    return std::span<const MeshData>(m_mesh_data.data(), m_mesh_data.size());
}

std::span<const float> StaticMeshGeometry::get_vertices() {
    return std::span<const float>(m_vertices.data(), m_vertices.size());
}

std::span<const float> StaticMeshGeometry::get_raw_vertices() {
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

} // namespace vat::geometry
