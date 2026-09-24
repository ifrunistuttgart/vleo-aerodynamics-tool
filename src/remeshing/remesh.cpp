#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include "remesh.h"

#include <cmath>
#include <stdexcept>
#include <string>

#include <CGAL/Polygon_mesh_processing/detect_features.h>
#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/remesh.h>

#include "cgal_mesh.h"
#include "repair.h"

namespace vat::remeshing {

namespace PMP = CGAL::Polygon_mesh_processing;

namespace {

// Area of an equilateral triangle per squared edge length, sqrt(3)/4.
constexpr double EQUILATERAL_AREA_PER_EDGE2 = 0.43301270189221935;

void ValidateOptions(const RemeshOptions& options) {
    const bool by_count = options.target_triangle_count > 0;
    const bool by_length = options.target_edge_length__m > 0.0f;
    if (by_count == by_length) {
        throw std::invalid_argument("give the remeshing size either as target_triangle_count or as "
            "target_edge_length__m: exactly one of them must be positive");
    }
    if (!std::isfinite(options.target_edge_length__m) || options.target_edge_length__m < 0.0f) {
        throw std::invalid_argument("target_edge_length__m must be a finite, non-negative length");
    }
    if (!(options.feature_angle__deg >= 0.0f && options.feature_angle__deg <= 180.0f)) {
        throw std::invalid_argument("feature_angle__deg must lie between 0 and 180");
    }
    if (options.iterations == 0) {
        throw std::invalid_argument("iterations must be at least 1");
    }
}

std::vector<unsigned int> TrianglesPerMesh(geometry::StaticMeshGeometry& geometry) {
    const auto counts = geometry.get_num_triangles_per_mesh();
    return std::vector<unsigned int>(counts.begin(), counts.end());
}

} // namespace

RemeshResult remesh(geometry::StaticMeshGeometry& geometry, const RemeshOptions& options) {
    ValidateOptions(options);
    if (geometry.get_num_triangles() == 0) {
        throw std::invalid_argument("cannot remesh a geometry without triangles");
    }

    const auto model_matrices = geometry.get_model_matrices();
    for (const glm::mat4& m : model_matrices) {
        if (m != glm::mat4(1.0f)) {
            SPDLOG_WARN("Remeshing a geometry with turned meshes: the meshes are remeshed unturned, "
                "and the new geometry starts unturned. Turn them again as needed.");
            break;
        }
    }

    RemeshReport report{};
    report.before = geometry::compute_mesh_quality(geometry);
    report.triangles_per_mesh_before = TrianglesPerMesh(geometry);

    // Repair everything before remeshing anything, so a mesh that cannot be repaired is
    // reported before minutes go into the others, and so the area behind a triangle-count
    // target is that of the repaired surface.
    const auto meshes = geometry.get_mesh_data();
    std::vector<SurfaceMesh> surfaces;
    surfaces.reserve(meshes.size());
    double total_area = 0.0;
    for (const geometry::MeshData& mesh : meshes) {
        RepairedMesh repaired = repair(mesh);
        report.repair += repaired.stats;
        total_area += CGAL::to_double(PMP::area(repaired.mesh));
        surfaces.push_back(std::move(repaired.mesh));
    }
    if (report.repair.changed()) {
        SPDLOG_INFO("Repaired before remeshing: {} vertices welded, {} unused vertices, {} duplicate "
            "and {} degenerate triangles removed, {} vertices split at non-manifold places",
            report.repair.merged_vertices, report.repair.removed_unused_vertices,
            report.repair.removed_duplicate_triangles, report.repair.removed_degenerate_triangles,
            report.repair.split_vertices);
    }

    const double edge_length = options.target_triangle_count > 0
        ? std::sqrt(total_area / (EQUILATERAL_AREA_PER_EDGE2 * options.target_triangle_count))
        : static_cast<double>(options.target_edge_length__m);
    report.target_edge_length__m = static_cast<float>(edge_length);
    SPDLOG_INFO("Remeshing {} meshes with target edge length {:.4g} m", surfaces.size(), edge_length);

    std::vector<geometry::MeshData> remeshed;
    remeshed.reserve(surfaces.size());
    for (std::size_t i = 0; i < surfaces.size(); ++i) {
        SurfaceMesh& surface = surfaces[i];

        // Sharp edges are constrained: split and collapsed only along themselves, never
        // flipped away, and their vertices are never smoothed off them. Borders of open
        // panels are constrained the same way by CGAL without being marked.
        auto [is_sharp, created] = surface.add_property_map<SurfaceMesh::Edge_index, bool>("e:vat_sharp", false);
        PMP::detect_sharp_edges(surface, static_cast<double>(options.feature_angle__deg), is_sharp);

        PMP::isotropic_remeshing(faces(surface), edge_length, surface,
            CGAL::parameters::number_of_iterations(options.iterations)
                .edge_is_constrained_map(is_sharp));

        geometry::MeshData& out = remeshed.emplace_back();
        out.name = meshes[i].name;
        from_surface_mesh(surface, out.positions, out.indices);
        if (out.indices.empty()) {
            throw std::runtime_error("remeshing left mesh " + std::to_string(i) + " (\"" + out.name
                + "\") without triangles");
        }
    }

    RemeshResult result;
    result.geometry = std::make_unique<geometry::RotatableMeshGeometry>(std::move(remeshed));
    report.after = geometry::compute_mesh_quality(*result.geometry);
    report.triangles_per_mesh_after = TrianglesPerMesh(*result.geometry);
    report.area_change__percent = 100.0f
        * (report.after.total_area__m2 - report.before.total_area__m2) / report.before.total_area__m2;

    SPDLOG_INFO("Remeshed {} -> {} triangles; median aspect ratio {:.3g} -> {:.3g}; area change {:+.3g} %",
        report.before.num_triangles, report.after.num_triangles, report.before.aspect_ratio.median,
        report.after.aspect_ratio.median, report.area_change__percent);

    result.report = std::move(report);
    return result;
}

} // namespace vat::remeshing
