#pragma once
#include <cstdint>
#include <span>
#include <vector>

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh.h>

namespace vat::remeshing {

/*
 * Conversion between the toolbox's flat indexed arrays and CGAL's halfedge mesh.
 *
 * This header exposes CGAL types, so it is internal to the remeshing module: the public
 * remeshing API takes and returns toolbox types only, and nothing outside src/remeshing
 * (apart from its tests) should include it.
 */

using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using SurfaceMesh = CGAL::Surface_mesh<Kernel::Point_3>;

/**
 * Builds a halfedge mesh from an indexed triangle list.
 *
 * @param positions Vertex positions, x/y/z triplets.
 * @param indices Three vertex indices per triangle, in winding order.
 * @return The mesh, with one face per input triangle in the same order and winding.
 * @throws std::invalid_argument if the index count is not a multiple of three, an index
 *         is out of range, or the triangles do not form a valid halfedge mesh (an edge
 *         shared by more than two triangles, or neighbours with opposing winding).
 */
SurfaceMesh to_surface_mesh(std::span<const float> positions, std::span<const std::uint32_t> indices);

/**
 * Flattens a halfedge mesh back into an indexed triangle list.
 *
 * Vertices and faces the mesh has marked as removed are skipped, and the remaining
 * vertices are numbered densely in iteration order.
 *
 * @param mesh A pure triangle mesh.
 * @param positions Output vertex positions, x/y/z triplets. Overwritten.
 * @param indices Output vertex indices, three per triangle. Overwritten.
 * @throws std::invalid_argument if a face is not a triangle.
 */
void from_surface_mesh(const SurfaceMesh& mesh, std::vector<float>& positions, std::vector<std::uint32_t>& indices);

} // namespace vat::remeshing
