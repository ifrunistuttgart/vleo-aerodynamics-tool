#pragma once
#include <cstddef>
#include <memory>
#include <vector>

#include "mesh_quality.h"
#include "repair_stats.h"
#include "rotatable_mesh_geometry.h"
#include "static_mesh_geometry.h"

namespace vat::remeshing {

/**
 * @brief How to remesh. Give the size either as a triangle count or as an edge length: set
 * exactly one of the two to a positive value.
 */
struct RemeshOptions {
    /**
     * @brief Approximate number of triangles for the whole geometry. Converted to an edge length
     * from the total surface area, so the actual count lands near it, not on it.
     */
    unsigned int target_triangle_count = 0;
    /** @brief Target edge length for every triangle of the geometry [m]; finite and >= 0. */
    float target_edge_length__m = 0.0f;
    /**
     * @brief Edges whose two faces meet at more than this dihedral angle count as sharp and are
     * kept exactly where they are, e.g. the rim of a panel or the edges of a box. They
     * shape the silhouette the flow sees, so rounding them would change the loads. Must lie
     * in [0, 180].
     */
    float feature_angle__deg = 30.0f;
    /** @brief Split/collapse/flip/relax passes, at least 1. More gives more uniform triangles. */
    unsigned int iterations = 3;
};

/**
 * @brief Above this many triangles remesh() logs a warning: every load evaluation then costs
 * tens of milliseconds in the per-triangle GSI loop alone.
 */
inline constexpr unsigned int REMESH_WARN_TRIANGLES = 250'000;
/** @brief remesh() never produces more triangles than this. */
inline constexpr unsigned int REMESH_MAX_TRIANGLES = 1'000'000;

/** @brief What remesh() would produce, worked out without remeshing. */
struct RemeshPrediction {
    /** @brief The edge length remesh() would use [m]. */
    float target_edge_length__m;
    /**
     * @brief Triangle count if the surface were tiled with ideal equilateral triangles. The real
     * count usually lands above it -- by about 5 % on geometry of a few large flat parts,
     * by 30 % or more with many small parts and sharp edges, which resist coarsening.
     */
    unsigned int predicted_triangles;
    /** @brief Host memory of the resulting geometry at predicted_triangles, roughly [bytes]. */
    std::size_t predicted_memory__bytes;
    /** @brief Total surface area after repair [m^2]. */
    float total_area__m2;
};

/** @brief What remesh() did, for checking that it did no harm. */
struct RemeshReport {
    /** @brief The edge length actually used, whichever way the size was given [m]. */
    float target_edge_length__m;
    /** @brief Quality of the input geometry. */
    geometry::MeshQuality before;
    /** @brief Quality of the remeshed geometry. */
    geometry::MeshQuality after;
    /**
     * @brief Total surface area after relative to before, in percent. Flat regions keep their
     * area exactly; only curved ones can change, as their facets are re-cut.
     */
    float area_change__percent;
    /** @brief Triangle count of each mesh of the input, in mesh order. */
    std::vector<unsigned int> triangles_per_mesh_before;
    /** @brief Triangle count of each mesh of the result, in mesh order. */
    std::vector<unsigned int> triangles_per_mesh_after;
    /** @brief Summed over all meshes. */
    RepairStats repair;
};

/** @brief Output of remesh(). */
struct RemeshResult {
    /** @brief The remeshed geometry, every mesh unturned; owned by the caller. */
    std::unique_ptr<geometry::RotatableMeshGeometry> geometry;
    /** @brief What remesh() did. */
    RemeshReport report;
};

/**
 * @brief Works out what remesh() would produce, without remeshing: cheap enough to try sizes.
 *
 * Runs the same repair as remesh(), so a mesh that cannot be remeshed is reported here too.
 *
 * @param geometry Geometry to remesh; not modified.
 * @param options Size and remeshing settings.
 * @return The edge length, predicted triangle count and memory, and the total area.
 * @throws std::invalid_argument under the same conditions as remesh(), except that a
 *         prediction above REMESH_MAX_TRIANGLES is returned rather than refused.
 */
RemeshPrediction predict_remesh(geometry::StaticMeshGeometry& geometry, const RemeshOptions& options);

/**
 * @brief Replaces a geometry's triangles with near-equilateral ones of one common size.
 *
 * Each mesh is repaired (see RepairStats) and then remeshed on its own, with the same
 * target edge length for all, so mesh count, order and names -- and therefore every
 * mesh_id -- stay the same. Sharp edges and the rims of open panels are held in place, and
 * no triangle's winding is reversed. The input is not modified.
 *
 * The meshes are remeshed in their untransformed frame, and the new geometry starts with
 * every mesh unturned: turn them again as needed.
 *
 * @param geometry Geometry to remesh; not modified.
 * @param options Size and remeshing settings.
 * @return The new geometry and a report comparing it with the input.
 * @throws std::invalid_argument if the options do not give exactly one positive size, are
 *         otherwise out of range (non-finite edge length, feature angle outside [0, 180],
 *         zero iterations), if the geometry has no triangles, if a mesh cannot be repaired
 *         without flipping triangles (see repair()), or if the predicted triangle count
 *         exceeds REMESH_MAX_TRIANGLES -- checked before any remeshing is done.
 * @throws std::runtime_error if the actual count exceeds REMESH_MAX_TRIANGLES after all,
 *         which the prediction can underestimate (see RemeshPrediction), or if remeshing
 *         leaves a mesh without triangles.
 */
RemeshResult remesh(geometry::StaticMeshGeometry& geometry, const RemeshOptions& options);

} // namespace vat::remeshing
