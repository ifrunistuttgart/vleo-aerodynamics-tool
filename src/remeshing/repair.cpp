#include "repair.h"

#include <array>
#include <stdexcept>
#include <string>
#include <vector>

#include <CGAL/Polygon_mesh_processing/orient_polygon_soup.h>
#include <CGAL/Polygon_mesh_processing/polygon_soup_to_polygon_mesh.h>
#include <CGAL/Polygon_mesh_processing/repair_degeneracies.h>
#include <CGAL/Polygon_mesh_processing/repair_polygon_soup.h>

namespace vat::remeshing {

namespace PMP = CGAL::Polygon_mesh_processing;

namespace {

using Point = Kernel::Point_3;
using Polygon = std::vector<std::size_t>;

// Positions of a triangle's corners, in order.
std::array<Point, 3> Corners(const std::vector<Point>& points, const Polygon& polygon) {
    return {points[polygon[0]], points[polygon[1]], points[polygon[2]]};
}

// Whether b lists the same corners as a in the opposite cyclic order. Corners are
// distinct here: welding and the pinched-triangle removal run before this is used.
bool IsReversed(const std::array<Point, 3>& a, const std::array<Point, 3>& b) {
    for (int shift = 0; shift < 3; ++shift) {
        if (a[0] == b[shift] && a[1] == b[(shift + 2) % 3] && a[2] == b[(shift + 1) % 3]) {
            return true;
        }
    }
    return false;
}

} // namespace

RepairedMesh repair(const geometry::MeshData& mesh) {
    RepairedMesh result;
    RepairStats& stats = result.stats;

    std::vector<Point> points;
    points.reserve(mesh.positions.size() / 3);
    for (std::size_t i = 0; i + 2 < mesh.positions.size(); i += 3) {
        points.emplace_back(mesh.positions[i], mesh.positions[i + 1], mesh.positions[i + 2]);
    }
    std::vector<Polygon> polygons;
    polygons.reserve(mesh.indices.size() / 3);
    for (std::size_t t = 0; t + 2 < mesh.indices.size(); t += 3) {
        polygons.push_back({mesh.indices[t], mesh.indices[t + 1], mesh.indices[t + 2]});
    }

    // Exact-position welding. Files often store one vertex per face corner (for flat
    // shading), which leaves every triangle an island until its corners are merged.
    stats.merged_vertices = PMP::merge_duplicate_points_in_polygon_soup(points, polygons);

    // Only now can a repeated corner show up, e.g. two corners that were separate vertices
    // at the same position.
    const std::size_t before_pinched = polygons.size();
    std::erase_if(polygons, [](const Polygon& p) { return p[0] == p[1] || p[1] == p[2] || p[2] == p[0]; });
    stats.removed_degenerate_triangles += before_pinched - polygons.size();

    // require_same_orientation: CGAL's default treats the front and back face of a
    // zero-thickness panel as duplicates and deletes one, and with it that side's force.
    stats.removed_duplicate_triangles = PMP::merge_duplicate_polygons_in_polygon_soup(points, polygons,
        CGAL::parameters::require_same_orientation(true));

    stats.removed_unused_vertices = PMP::remove_isolated_points_in_polygon_soup(points, polygons);

    if (!PMP::is_polygon_soup_a_polygon_mesh(polygons)) {
        // Untangle non-manifold edges and vertices by duplicating points. CGAL may also
        // reverse triangles to make neighbours agree; refuse that instead of accepting it.
        std::vector<std::array<Point, 3>> before;
        before.reserve(polygons.size());
        for (const Polygon& p : polygons) {
            before.push_back(Corners(points, p));
        }
        const std::size_t points_before = points.size();

        PMP::orient_polygon_soup(points, polygons);

        std::size_t reversed = 0;
        for (std::size_t i = 0; i < polygons.size(); ++i) {
            if (IsReversed(before[i], Corners(points, polygons[i]))) {
                ++reversed;
            }
        }
        if (reversed > 0) {
            throw std::invalid_argument("mesh \"" + mesh.name + "\": " + std::to_string(reversed)
                + " of " + std::to_string(polygons.size()) + " triangles are wound opposite to the "
                "neighbours they share an edge with. The winding decides which side of a triangle "
                "is loaded, so these already get the wrong sign of force. Make the face orientation "
                "consistent in the modelling tool and export again.");
        }
        stats.split_vertices = points.size() - points_before;

        if (!PMP::is_polygon_soup_a_polygon_mesh(polygons)) {
            throw std::invalid_argument("mesh \"" + mesh.name + "\" could not be turned into a "
                "valid surface mesh");
        }
    }

    PMP::polygon_soup_to_polygon_mesh(points, polygons, result.mesh);

    // Collinear corners: zero area but three distinct vertices. Removing them collapses or
    // flips edges, which leaves the surface and its area where they were.
    const std::size_t faces_before = result.mesh.number_of_faces();
    PMP::remove_degenerate_faces(result.mesh);
    stats.removed_degenerate_triangles += faces_before - result.mesh.number_of_faces();
    result.mesh.collect_garbage();

    return result;
}

} // namespace vat::remeshing
