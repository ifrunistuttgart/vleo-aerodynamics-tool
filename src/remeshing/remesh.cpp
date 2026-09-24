#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include "remesh.h"

#include <cmath>
#include <limits>
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

// Host memory per triangle of a RotatableMeshGeometry: flattened vertices (36 B), IDs (12),
// normals (12), centroids (12) and areas (4), their transformed copies (60), and the
// indexed MeshData kept alongside (12 B of indices plus ~6 B of shared positions).
constexpr std::size_t BYTES_PER_TRIANGLE = 154;

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

// Everything remesh() and predict_remesh() have in common: the repaired meshes and the
// edge length that follows from the options.
struct Prepared {
    std::vector<SurfaceMesh> surfaces;
    RepairStats repair;
    double total_area__m2 = 0.0;
    double edge_length__m = 0.0;
    double predicted_triangles = 0.0;
};

Prepared Prepare(geometry::StaticMeshGeometry& geometry, const RemeshOptions& options) {
    ValidateOptions(options);
    if (geometry.get_num_triangles() == 0) {
        throw std::invalid_argument("cannot remesh a geometry without triangles");
    }

    // Repair everything before remeshing anything, so a mesh that cannot be repaired is
    // reported before minutes go into the others, and so the area behind a triangle-count
    // target is that of the repaired surface.
    Prepared prepared;
    const auto meshes = geometry.get_mesh_data();
    prepared.surfaces.reserve(meshes.size());
    for (const geometry::MeshData& mesh : meshes) {
        RepairedMesh repaired = repair(mesh);
        prepared.repair += repaired.stats;
        prepared.total_area__m2 += CGAL::to_double(PMP::area(repaired.mesh));
        prepared.surfaces.push_back(std::move(repaired.mesh));
    }

    prepared.edge_length__m = options.target_triangle_count > 0
        ? std::sqrt(prepared.total_area__m2 / (EQUILATERAL_AREA_PER_EDGE2 * options.target_triangle_count))
        : static_cast<double>(options.target_edge_length__m);
    prepared.predicted_triangles = prepared.total_area__m2
        / (EQUILATERAL_AREA_PER_EDGE2 * prepared.edge_length__m * prepared.edge_length__m);
    return prepared;
}

RemeshPrediction ToPrediction(const Prepared& prepared) {
    RemeshPrediction prediction{};
    prediction.target_edge_length__m = static_cast<float>(prepared.edge_length__m);
    // Saturate rather than overflow: an absurdly small edge length must still read as huge.
    const double capped = std::min(prepared.predicted_triangles,
        static_cast<double>(std::numeric_limits<unsigned int>::max()));
    prediction.predicted_triangles = static_cast<unsigned int>(std::llround(capped));
    prediction.predicted_memory__bytes = static_cast<std::size_t>(prediction.predicted_triangles) * BYTES_PER_TRIANGLE;
    prediction.total_area__m2 = static_cast<float>(prepared.total_area__m2);
    return prediction;
}

} // namespace

RemeshPrediction predict_remesh(geometry::StaticMeshGeometry& geometry, const RemeshOptions& options) {
    return ToPrediction(Prepare(geometry, options));
}

RemeshResult remesh(geometry::StaticMeshGeometry& geometry, const RemeshOptions& options) {
    Prepared prepared = Prepare(geometry, options);
    const RemeshPrediction prediction = ToPrediction(prepared);

    if (prepared.predicted_triangles > REMESH_MAX_TRIANGLES) {
        throw std::invalid_argument("remeshing with edge length " + std::to_string(prediction.target_edge_length__m)
            + " m would give about " + std::to_string(prediction.predicted_triangles) + " triangles, more than the "
            "limit of " + std::to_string(REMESH_MAX_TRIANGLES) + ". Ask for fewer triangles or a longer edge.");
    }
    if (prepared.predicted_triangles > REMESH_WARN_TRIANGLES) {
        SPDLOG_WARN("Remeshing to about {} triangles (~{} MB). Every load evaluation will spend tens of "
            "milliseconds in the per-triangle GSI loop; consider fewer triangles.",
            prediction.predicted_triangles, prediction.predicted_memory__bytes / 1'000'000);
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
    report.repair = prepared.repair;
    report.target_edge_length__m = prediction.target_edge_length__m;
    if (report.repair.changed()) {
        SPDLOG_INFO("Repaired before remeshing: {} vertices welded, {} unused vertices, {} duplicate "
            "and {} degenerate triangles removed, {} vertices split at non-manifold places",
            report.repair.merged_vertices, report.repair.removed_unused_vertices,
            report.repair.removed_duplicate_triangles, report.repair.removed_degenerate_triangles,
            report.repair.split_vertices);
    }
    SPDLOG_INFO("Remeshing {} meshes with target edge length {:.4g} m (about {} triangles)",
        prepared.surfaces.size(), prepared.edge_length__m, prediction.predicted_triangles);

    const auto meshes = geometry.get_mesh_data();
    std::vector<geometry::MeshData> remeshed;
    remeshed.reserve(prepared.surfaces.size());
    std::size_t total_triangles = 0;
    for (std::size_t i = 0; i < prepared.surfaces.size(); ++i) {
        SurfaceMesh& surface = prepared.surfaces[i];

        // Sharp edges are constrained: split and collapsed only along themselves, never
        // flipped away, and their vertices are never smoothed off them. Borders of open
        // panels are constrained the same way by CGAL without being marked.
        auto [is_sharp, created] = surface.add_property_map<SurfaceMesh::Edge_index, bool>("e:vat_sharp", false);
        PMP::detect_sharp_edges(surface, static_cast<double>(options.feature_angle__deg), is_sharp);

        PMP::isotropic_remeshing(faces(surface), prepared.edge_length__m, surface,
            CGAL::parameters::number_of_iterations(options.iterations)
                .edge_is_constrained_map(is_sharp));

        geometry::MeshData& out = remeshed.emplace_back();
        out.name = meshes[i].name;
        from_surface_mesh(surface, out.positions, out.indices);
        if (out.indices.empty()) {
            throw std::runtime_error("remeshing left mesh " + std::to_string(i) + " (\"" + out.name
                + "\") without triangles");
        }
        total_triangles += out.indices.size() / 3;
    }

    // The prediction ignores how sharp edges resist coarsening, so it can come in low.
    if (total_triangles > REMESH_MAX_TRIANGLES) {
        throw std::runtime_error("remeshing produced " + std::to_string(total_triangles) + " triangles, more "
            "than the limit of " + std::to_string(REMESH_MAX_TRIANGLES) + " (predicted "
            + std::to_string(prediction.predicted_triangles) + "). Ask for about "
            + std::to_string(static_cast<unsigned long long>(prediction.predicted_triangles
                * (static_cast<double>(REMESH_MAX_TRIANGLES) / total_triangles) * 0.95))
            + " triangles or fewer.");
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
