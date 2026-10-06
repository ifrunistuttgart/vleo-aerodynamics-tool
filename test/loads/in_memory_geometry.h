#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "Igeometry_shading_data.h"

// Test geometry built in code: one mesh, identity model matrix, de-indexed triangles.
// Normals follow the winding (counter-clockwise seen from the side they point to),
// exactly as StaticMeshGeometry computes them.
class InMemoryGeometry final : public vat::IGeometryShadingData {
public:
    InMemoryGeometry() = default;

    void add_triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) {
        const std::uint32_t id = static_cast<std::uint32_t>(m_areas.size() + 1);
        for (const glm::vec3& p : {a, b, c}) {
            m_vertices.insert(m_vertices.end(), {p.x, p.y, p.z});
            m_ids.push_back(id);
            m_radius = std::max(m_radius, glm::length(p));
        }
        const glm::vec3 cross = glm::cross(b - a, c - a);
        const glm::vec3 normal = glm::normalize(cross);
        const glm::vec3 centroid = (a + b + c) / 3.0f;
        m_normals.insert(m_normals.end(), {normal.x, normal.y, normal.z});
        m_centroids.insert(m_centroids.end(), {centroid.x, centroid.y, centroid.z});
        m_areas.push_back(0.5f * glm::length(cross));
        m_num_triangles_per_mesh[0] = static_cast<unsigned int>(m_areas.size());
    }

    // Quad p0 p1 p2 p3 (counter-clockwise), split into cells x cells squares of two
    // triangles each.
    void add_quad(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, int cells = 1) {
        auto at = [&](int i, int j) {
            const float u = static_cast<float>(i) / cells;
            const float v = static_cast<float>(j) / cells;
            return (1 - u) * (1 - v) * p0 + u * (1 - v) * p1 + u * v * p2 + (1 - u) * v * p3;
        };
        for (int j = 0; j < cells; ++j) {
            for (int i = 0; i < cells; ++i) {
                add_triangle(at(i, j), at(i + 1, j), at(i + 1, j + 1));
                add_triangle(at(i, j), at(i + 1, j + 1), at(i, j + 1));
            }
        }
    }

    // Closed axis-aligned box, faces pointing outwards.
    void add_box(const glm::vec3& lo, const glm::vec3& hi, int cells = 1) {
        const glm::vec3 c[8] = {
            {lo.x, lo.y, lo.z}, {hi.x, lo.y, lo.z}, {hi.x, hi.y, lo.z}, {lo.x, hi.y, lo.z},
            {lo.x, lo.y, hi.z}, {hi.x, lo.y, hi.z}, {hi.x, hi.y, hi.z}, {lo.x, hi.y, hi.z},
        };
        add_quad(c[0], c[3], c[2], c[1], cells); // -z
        add_quad(c[4], c[5], c[6], c[7], cells); // +z
        add_quad(c[0], c[1], c[5], c[4], cells); // -y
        add_quad(c[3], c[7], c[6], c[2], cells); // +y
        add_quad(c[0], c[4], c[7], c[3], cells); // -x
        add_quad(c[1], c[2], c[6], c[5], cells); // +x
    }

    // Sphere approximated by an octahedron whose faces are split `levels` times.
    void add_sphere(float radius, int levels) {
        const glm::vec3 x(1, 0, 0), y(0, 1, 0), z(0, 0, 1);
        const glm::vec3 faces[8][3] = {
            {x, y, z}, {y, -x, z}, {-x, -y, z}, {-y, x, z},
            {y, x, -z}, {-x, y, -z}, {-y, -x, -z}, {x, -y, -z},
        };
        for (const auto& f : faces) {
            add_sphere_face(f[0], f[1], f[2], radius, levels);
        }
    }

    std::span<const float> get_vertices() override { return m_vertices; }
    std::span<const float> get_raw_vertices() override { return m_vertices; }
    std::span<const std::uint32_t> get_triangle_ids() override { return m_ids; }
    std::span<const float> get_normals() override { return m_normals; }
    std::span<const float> get_raw_normals() override { return m_normals; }
    std::span<const float> get_areas() override { return m_areas; }
    std::span<const float> get_centroids() override { return m_centroids; }
    std::span<const glm::mat4> get_model_matrices() override { return m_model_matrices; }
    std::span<const std::string> get_mesh_names() override { return m_mesh_names; }
    std::span<const unsigned int> get_num_triangles_per_mesh() override { return m_num_triangles_per_mesh; }
    const unsigned int get_num_triangles() override { return static_cast<unsigned int>(m_areas.size()); }
    float get_bounding_sphere_radius() override { return m_radius; }

private:
    void add_sphere_face(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float radius, int levels) {
        if (levels == 0) {
            add_triangle(radius * a, radius * b, radius * c);
            return;
        }
        const glm::vec3 ab = glm::normalize(a + b);
        const glm::vec3 bc = glm::normalize(b + c);
        const glm::vec3 ca = glm::normalize(c + a);
        add_sphere_face(a, ab, ca, radius, levels - 1);
        add_sphere_face(ab, b, bc, radius, levels - 1);
        add_sphere_face(ca, bc, c, radius, levels - 1);
        add_sphere_face(ab, bc, ca, radius, levels - 1);
    }

    std::vector<float> m_vertices;
    std::vector<std::uint32_t> m_ids;
    std::vector<float> m_normals;
    std::vector<float> m_centroids;
    std::vector<float> m_areas;
    std::vector<glm::mat4> m_model_matrices{glm::mat4(1.0f)};
    std::vector<std::string> m_mesh_names{"Mesh 0"};
    std::vector<unsigned int> m_num_triangles_per_mesh{0};
    float m_radius = 0.0f;
};
