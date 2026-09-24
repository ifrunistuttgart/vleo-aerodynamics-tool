#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "cgal_mesh.h"

using namespace vat::remeshing;

namespace {

// A closed tetrahedron with outward winding. Coordinates are exactly representable in
// float, so a float -> double -> float round trip must reproduce them bit for bit.
const std::vector<float> TETRA_POSITIONS = {
    0.0f, -0.5f, 0.0f,
    0.0f,  0.5f, 0.0f,
    0.0f,  0.0f, 0.5f,
   -0.5f,  0.0f, 0.0f,
};
const std::vector<std::uint32_t> TETRA_INDICES = {
    0, 3, 1,
    1, 3, 2,
    2, 3, 0,
    0, 1, 2,
};

} // namespace

TEST(CgalMeshTest, RoundTripPreservesPositionsIndicesAndWinding) {
    const SurfaceMesh mesh = to_surface_mesh(TETRA_POSITIONS, TETRA_INDICES);
    EXPECT_EQ(mesh.number_of_vertices(), 4u);
    EXPECT_EQ(mesh.number_of_faces(), 4u);
    EXPECT_TRUE(CGAL::is_closed(mesh));

    std::vector<float> positions;
    std::vector<std::uint32_t> indices;
    from_surface_mesh(mesh, positions, indices);

    EXPECT_EQ(positions, TETRA_POSITIONS);
    EXPECT_EQ(indices, TETRA_INDICES);
}

TEST(CgalMeshTest, OpenMeshIsAccepted) {
    // A single panel: two triangles sharing one edge, with a border all around.
    const std::vector<float> positions = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0};
    const std::vector<std::uint32_t> indices = {0, 1, 2, 0, 2, 3};

    const SurfaceMesh mesh = to_surface_mesh(positions, indices);
    EXPECT_EQ(mesh.number_of_faces(), 2u);
    EXPECT_FALSE(CGAL::is_closed(mesh));
}

TEST(CgalMeshTest, EdgeSharedByThreeTrianglesIsRejected) {
    // Three fins on the common edge 0-1.
    const std::vector<float> positions = {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 1};
    const std::vector<std::uint32_t> indices = {0, 1, 2, 1, 0, 3, 0, 1, 4};

    EXPECT_THROW(to_surface_mesh(positions, indices), std::invalid_argument);
}

TEST(CgalMeshTest, OpposingWindingIsRejected) {
    // Both triangles traverse the shared edge 0->2 in the same direction.
    const std::vector<float> positions = {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0};
    const std::vector<std::uint32_t> indices = {0, 1, 2, 0, 3, 2};
    std::vector<std::uint32_t> flipped = {0, 1, 2, 0, 2, 3};
    ASSERT_NO_THROW(to_surface_mesh(positions, flipped));

    EXPECT_THROW(to_surface_mesh(positions, indices), std::invalid_argument);
}

TEST(CgalMeshTest, MalformedInputIsRejected) {
    EXPECT_THROW(to_surface_mesh(std::vector<float>{0, 0}, TETRA_INDICES), std::invalid_argument);
    EXPECT_THROW(to_surface_mesh(TETRA_POSITIONS, std::vector<std::uint32_t>{0, 1}), std::invalid_argument);
    EXPECT_THROW(to_surface_mesh(TETRA_POSITIONS, std::vector<std::uint32_t>{0, 1, 4}), std::invalid_argument);
}

TEST(CgalMeshTest, RemovedVerticesAreSkipped) {
    SurfaceMesh mesh = to_surface_mesh(TETRA_POSITIONS, TETRA_INDICES);
    // An isolated vertex that is then removed leaves a gap in the raw indices, which is
    // what a remeshing pass does at scale when it collapses edges.
    const auto stray = mesh.add_vertex(Kernel::Point_3(9, 9, 9));
    mesh.remove_vertex(stray);
    ASSERT_TRUE(mesh.has_garbage());

    std::vector<float> positions;
    std::vector<std::uint32_t> indices;
    from_surface_mesh(mesh, positions, indices);

    EXPECT_EQ(positions, TETRA_POSITIONS);
    EXPECT_EQ(indices, TETRA_INDICES);
}
