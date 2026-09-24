#pragma once
#include <memory>
#include <vector>

#include "mesh_quality.h"
#include "repair_stats.h"
#include "rotatable_mesh_geometry.h"
#include "static_mesh_geometry.h"

namespace vat::remeshing {

/**
 * How to remesh. Give the size either as a triangle count or as an edge length: set
 * exactly one of the two to a positive value.
 */
struct RemeshOptions {
    /**
     * Approximate number of triangles for the whole geometry. Converted to an edge length
     * from the total surface area, so the actual count lands near it, not on it.
     */
    unsigned int target_triangle_count = 0;
    /** Target edge length for every triangle of the geometry [m]. */
    float target_edge_length__m = 0.0f;
    /**
     * Edges whose two faces meet at more than this dihedral angle count as sharp and are
     * kept exactly where they are, e.g. the rim of a panel or the edges of a box. They
     * shape the silhouette the flow sees, so rounding them would change the loads.
     */
    float feature_angle__deg = 30.0f;
    /** Split/collapse/flip/relax passes. More gives more uniform triangles. */
    unsigned int iterations = 3;
};

/** What remesh() did, for checking that it did no harm. */
struct RemeshReport {
    /** The edge length actually used, whichever way the size was given [m]. */
    float target_edge_length__m;
    geometry::MeshQuality before;
    geometry::MeshQuality after;
    /**
     * Total surface area after relative to before, in percent. Flat regions keep their
     * area exactly; only curved ones can change, as their facets are re-cut.
     */
    float area_change__percent;
    std::vector<unsigned int> triangles_per_mesh_before;
    std::vector<unsigned int> triangles_per_mesh_after;
    /** Summed over all meshes. */
    RepairStats repair;
};

struct RemeshResult {
    std::unique_ptr<geometry::RotatableMeshGeometry> geometry;
    RemeshReport report;
};

/**
 * Replaces a geometry's triangles with near-equilateral ones of one common size.
 *
 * Each mesh is repaired (see RepairStats) and then remeshed on its own, with the same
 * target edge length for all, so mesh count, order and names -- and therefore every
 * mesh_id -- stay the same. Sharp edges and the rims of open panels are held in place, and
 * no triangle's winding is reversed. The input is not modified.
 *
 * The meshes are remeshed in their untransformed frame, and the new geometry starts with
 * every mesh unturned: turn them again as needed.
 *
 * @throws std::invalid_argument if the options do not give exactly one positive size, if
 *         the geometry has no triangles, or if a mesh cannot be repaired without flipping
 *         triangles (see repair()).
 */
RemeshResult remesh(geometry::StaticMeshGeometry& geometry, const RemeshOptions& options);

} // namespace vat::remeshing
