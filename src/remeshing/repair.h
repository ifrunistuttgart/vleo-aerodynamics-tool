#pragma once

#include "cgal_mesh.h"
#include "mesh_data.h"
#include "repair_stats.h"

namespace vat::remeshing {

/*
 * Turning an imported mesh into a valid halfedge mesh that remeshing can work on.
 *
 * Internal to the remeshing module, like cgal_mesh.h: it hands back a CGAL mesh.
 */

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
