#include "cgal_mesh.h"

#include <array>
#include <stdexcept>
#include <string>

#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>

namespace vat::remeshing {

namespace PMP = CGAL::Polygon_mesh_processing;

SurfaceMesh to_surface_mesh(std::span<const float> positions, std::span<const std::uint32_t> indices) {
    if (positions.size() % 3 != 0) {
        throw std::invalid_argument("positions must hold x/y/z triplets, got "
            + std::to_string(positions.size()) + " floats");
    }
    if (indices.size() % 3 != 0) {
        throw std::invalid_argument("indices must hold three entries per triangle, got "
            + std::to_string(indices.size()));
    }

    const std::size_t num_vertices = positions.size() / 3;
    std::vector<Kernel::Point_3> points;
    points.reserve(num_vertices);
    for (std::size_t i = 0; i < num_vertices; ++i) {
        points.emplace_back(positions[3 * i], positions[3 * i + 1], positions[3 * i + 2]);
    }

    std::vector<std::array<std::size_t, 3>> triangles;
    triangles.reserve(indices.size() / 3);
    for (std::size_t t = 0; t < indices.size(); t += 3) {
        for (std::size_t k = 0; k < 3; ++k) {
            if (indices[t + k] >= num_vertices) {
                throw std::invalid_argument("triangle " + std::to_string(t / 3)
                    + " references vertex " + std::to_string(indices[t + k])
                    + ", but there are only " + std::to_string(num_vertices));
            }
        }
        triangles.push_back({indices[t], indices[t + 1], indices[t + 2]});
    }

    // Going through the soup API rather than add_face() gives an up-front yes/no on
    // whether the triangles form a halfedge mesh at all, instead of a half-built mesh.
    if (!PMP::is_polygon_soup_a_polygon_mesh(triangles)) {
        throw std::invalid_argument("triangles do not form a valid halfedge mesh: an edge is "
            "shared by more than two triangles, or neighbouring triangles have opposing winding");
    }

    SurfaceMesh mesh;
    PMP::polygon_soup_to_polygon_mesh(points, triangles, mesh);
    return mesh;
}

void from_surface_mesh(const SurfaceMesh& mesh, std::vector<float>& positions, std::vector<std::uint32_t>& indices) {
    positions.clear();
    indices.clear();
    positions.reserve(3 * mesh.number_of_vertices());
    indices.reserve(3 * mesh.number_of_faces());

    // Surface_mesh keeps removed elements as garbage until collect_garbage(), so its raw
    // indices can have gaps. Renumber densely instead of mutating a const mesh.
    std::vector<std::uint32_t> dense_index(mesh.number_of_vertices() + mesh.number_of_removed_vertices());
    std::uint32_t next = 0;
    for (const auto v : mesh.vertices()) {
        dense_index[static_cast<std::size_t>(v)] = next++;
        const auto& p = mesh.point(v);
        positions.push_back(static_cast<float>(p.x()));
        positions.push_back(static_cast<float>(p.y()));
        positions.push_back(static_cast<float>(p.z()));
    }

    for (const auto f : mesh.faces()) {
        if (mesh.degree(f) != 3) {
            throw std::invalid_argument("face " + std::to_string(static_cast<std::size_t>(f))
                + " is not a triangle");
        }
        for (const auto v : CGAL::vertices_around_face(mesh.halfedge(f), mesh)) {
            indices.push_back(dense_index[static_cast<std::size_t>(v)]);
        }
    }
}

} // namespace vat::remeshing
