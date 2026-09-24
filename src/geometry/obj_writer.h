#pragma once
#include <string>

#include "static_mesh_geometry.h"

namespace vat::geometry {

/**
 * Writes a geometry to a Wavefront .obj file that the toolbox loads back unchanged.
 *
 * Each mesh becomes one `o` object, in mesh_id order and with its name, so hinge
 * definitions written against the original file keep addressing the same parts.
 * Whitespace in a name is replaced by '_' (with a warning), since an `o` name ends at the
 * first whitespace on reload. The untransformed meshes are written (see get_mesh_data()),
 * with their winding.
 *
 * Reading the file back reproduces every coordinate to within one float ulp (~6e-8
 * relative), not bit for bit: assimp's .obj parser is not correctly rounded, so no decimal
 * spelling round-trips exactly. Repeated export and reload can therefore drift by a few
 * ulps.
 *
 * @param geometry The geometry to write.
 * @param file Path of the .obj to create or overwrite.
 * @throws std::invalid_argument if the geometry has no triangles, if a mesh has no
 *         triangles (it would vanish on reload and shift every later mesh_id), or if any
 *         mesh is turned away from its original pose. Writing a turned geometry would
 *         bake the rotation into the vertices, and every hinge angle applied to the
 *         reloaded file would then act on top of it.
 * @throws std::runtime_error if the file cannot be written.
 */
void write_obj(StaticMeshGeometry& geometry, const std::string& file);

} // namespace vat::geometry
