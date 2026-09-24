#pragma once
#include <cstddef>

#include "cgal_mesh.h"
#include "mesh_data.h"

namespace vat::remeshing {

/*
 * Turning an imported mesh into a valid halfedge mesh that remeshing can work on.
 *
 * Internal to the remeshing module, like cgal_mesh.h: it hands back a CGAL mesh.
 */

/** What repair() changed. All zero means the mesh was already clean. */
struct RepairStats {
    /** Vertices removed because another vertex sat at exactly the same position. */
    std::size_t merged_vertices = 0;
    /** Vertices no triangle referenced. */
    std::size_t removed_unused_vertices = 0;
    /** Triangles that repeated another one exactly, same winding included. */
    std::size_t removed_duplicate_triangles = 0;
    /** Triangles with zero area, either a repeated corner or three collinear corners. */
    std::size_t removed_degenerate_triangles = 0;
    /**
     * Vertices duplicated to pull apart places where the surface is not a manifold,
     * e.g. an edge shared by three triangles where a fin stands on a plate. The geometry
     * is unchanged; the parts just stop being connected there.
     */
    std::size_t split_vertices = 0;

    bool changed() const {
        return merged_vertices + removed_unused_vertices + removed_duplicate_triangles
            + removed_degenerate_triangles + split_vertices > 0;
    }
};

struct RepairedMesh {
    SurfaceMesh mesh;
    RepairStats stats;
};

/**
 * Welds, cleans and untangles one mesh so it forms a valid halfedge mesh, without
 * changing its shape or the winding of any triangle.
 *
 * Winding is never changed because it is what makes a triangle windward or leeward:
 * reversing one would reverse the force on it. Two coincident triangles with opposite
 * winding -- the two sides of a zero-thickness panel -- are therefore both kept.
 *
 * @param mesh One mesh of a geometry, in its own frame.
 * @return The repaired mesh and what was changed to get it.
 * @throws std::invalid_argument if some triangles are wound opposite to the neighbours
 *         they share an edge with, since no consistent orientation exists without
 *         flipping some of them, and which ones is not ours to guess.
 */
RepairedMesh repair(const geometry::MeshData& mesh);

} // namespace vat::remeshing
