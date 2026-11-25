#pragma once
#include <cstddef>

namespace vat::remeshing {

/** @brief What repair() changed. All zero means the mesh was already clean. */
struct RepairStats {
    /** @brief Vertices removed because another vertex sat at exactly the same position. */
    std::size_t merged_vertices = 0;
    /** @brief Vertices no triangle referenced. */
    std::size_t removed_unused_vertices = 0;
    /** @brief Triangles that repeated another one exactly, same winding included. */
    std::size_t removed_duplicate_triangles = 0;
    /** @brief Triangles with zero area, either a repeated corner or three collinear corners. */
    std::size_t removed_degenerate_triangles = 0;
    /**
     * @brief Vertices duplicated to pull apart places where the surface is not a manifold,
     * e.g. an edge shared by three triangles where a fin stands on a plate. The geometry
     * is unchanged; the parts just stop being connected there.
     */
    std::size_t split_vertices = 0;

    /** @brief Whether repair() changed anything, i.e. any count is non-zero. */
    bool changed() const {
        return merged_vertices + removed_unused_vertices + removed_duplicate_triangles + removed_degenerate_triangles +
                   split_vertices >
               0;
    }
};

/**
 * @brief Adds the counts of s to total, e.g. to sum the stats of all meshes.
 * @param total Accumulator.
 * @param s Stats to add.
 * @return total.
 */
inline RepairStats& operator+=(RepairStats& total, const RepairStats& s) {
    total.merged_vertices += s.merged_vertices;
    total.removed_unused_vertices += s.removed_unused_vertices;
    total.removed_duplicate_triangles += s.removed_duplicate_triangles;
    total.removed_degenerate_triangles += s.removed_degenerate_triangles;
    total.split_vertices += s.split_vertices;
    return total;
}

} // namespace vat::remeshing
