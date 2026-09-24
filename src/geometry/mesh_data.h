#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace vat::geometry {

/**
 * One mesh of a geometry as an indexed triangle list, in its own (untransformed) frame.
 *
 * This is the connectivity-preserving form a geometry is built from. The geometry itself
 * flattens it into one explicit vertex triple per triangle for rendering, which loses the
 * information about which triangles share a vertex; anything that needs that information
 * -- remeshing, export -- works on MeshData instead.
 */
struct MeshData {
    /** Name of the mesh, e.g. the `o` name in an .obj. May be empty. */
    std::string name;
    /** Vertex positions, x/y/z triplets [m]. */
    std::vector<float> positions;
    /** Three indices into positions per triangle, in winding order. */
    std::vector<std::uint32_t> indices;
};

/**
 * Loads every mesh of a model file, one MeshData per mesh, in file order.
 *
 * Faces are triangulated and identical vertices joined on import; faces that are not
 * triangles after that (points, lines) are dropped.
 *
 * @param file Path to any format assimp reads.
 * @return The meshes, or an empty vector if the file cannot be read. The failure is logged.
 */
std::vector<MeshData> load_mesh_data(const std::string& file);

} // namespace vat::geometry
