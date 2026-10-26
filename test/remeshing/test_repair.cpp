#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <ostream>
#include <stdexcept>
#include <vector>

#include "cgal_mesh.h"
#include "mesh_data.h"
#include "mesh_quality.h"
#include "repair.h"
#include "static_mesh_geometry.h"
#include "test_helpers.h"

namespace vat::remeshing {
// Found by argument-dependent lookup, so gtest can print stats in failure messages.
std::ostream& operator<<(std::ostream& os, const RepairStats& s) {
    return os << "merged " << s.merged_vertices << ", unused " << s.removed_unused_vertices
              << ", duplicate " << s.removed_duplicate_triangles << ", degenerate "
              << s.removed_degenerate_triangles << ", split " << s.split_vertices;
}
} // namespace vat::remeshing

using namespace vat::remeshing;
using vat::geometry::MeshData;
using vat::geometry::StaticMeshGeometry;

namespace {

using Corner = std::array<float, 3>;
using Triangle = std::array<Corner, 3>;

// Each triangle with its corners rotated so the smallest comes first. Rotation keeps the
// winding, so two lists compare equal exactly when they hold the same triangles, wound
// the same way, in the same order.
std::vector<Triangle> CanonicalTriangles(const MeshData& mesh) {
    std::vector<Triangle> out;
    for (std::size_t t = 0; t < mesh.indices.size(); t += 3) {
        Triangle tri;
        for (int k = 0; k < 3; ++k) {
            const float* p = &mesh.positions[3 * mesh.indices[t + k]];
            tri[k] = {p[0], p[1], p[2]};
        }
        std::rotate(tri.begin(), std::min_element(tri.begin(), tri.end()), tri.end());
        out.push_back(tri);
    }
    return out;
}

MeshData Flatten(const SurfaceMesh& mesh, std::string name = "m") {
    MeshData out{std::move(name), {}, {}};
    from_surface_mesh(mesh, out.positions, out.indices);
    return out;
}

// A unit square in z = 0, split along its diagonal, wound towards +z.
MeshData Square() {
    return MeshData{"square", {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0}, {0, 1, 2, 0, 2, 3}};
}

std::vector<MeshData> Load(const char* relative_path) {
    return vat::geometry::load_mesh_data(GetTestDataPath(__FILE__, relative_path));
}

} // namespace

TEST(RepairTest, CleanMeshIsLeftAlone) {
    for (const MeshData& mesh : Load("../../matlab/examples/geometries/shuttlecock_960.obj")) {
        const RepairedMesh repaired = repair(mesh);
        EXPECT_FALSE(repaired.stats.changed()) << mesh.name;
        EXPECT_EQ(CanonicalTriangles(Flatten(repaired.mesh)), CanonicalTriangles(mesh)) << mesh.name;
    }
}

TEST(RepairTest, OpenMeshKeepsItsSurface) {
    // soar_satellite: open panels with borders, and each mesh has vertices where surface
    // pieces touch at a single point. Those get split apart -- no vertex moves -- but
    // nothing may be merged, removed or flipped. Its vertices shared between meshes are
    // not welded either: repair works per mesh, and welding across meshes would glue
    // parts that turn independently.
    std::vector<MeshData> original = Load("../../matlab/examples/geometries/soar_satellite.obj");
    std::vector<MeshData> repaired_meshes;
    std::size_t split = 0;
    for (const MeshData& mesh : original) {
        const RepairedMesh repaired = repair(mesh);
        split += repaired.stats.split_vertices;
        EXPECT_EQ(repaired.stats.merged_vertices, 0u) << mesh.name << ": " << repaired.stats;
        EXPECT_EQ(repaired.stats.removed_duplicate_triangles, 0u) << mesh.name << ": " << repaired.stats;
        EXPECT_EQ(repaired.stats.removed_degenerate_triangles, 0u) << mesh.name << ": " << repaired.stats;
        repaired_meshes.push_back(Flatten(repaired.mesh, mesh.name));
        EXPECT_EQ(CanonicalTriangles(repaired_meshes.back()), CanonicalTriangles(mesh)) << mesh.name;
    }

    EXPECT_GT(split, 0u);

    StaticMeshGeometry before(std::move(original));
    StaticMeshGeometry after(std::move(repaired_meshes));
    EXPECT_EQ(compute_mesh_quality(after).total_area__m2, compute_mesh_quality(before).total_area__m2);
}

TEST(RepairTest, CornersStoredPerFaceAreWelded) {
    // The square again, but with the diagonal's two vertices stored once per triangle.
    const MeshData mesh{"square", {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0}, {0, 1, 2, 3, 4, 5}};
    const RepairedMesh repaired = repair(mesh);

    EXPECT_EQ(repaired.stats.merged_vertices, 2u);
    EXPECT_EQ(repaired.mesh.number_of_vertices(), 4u);
    EXPECT_EQ(repaired.mesh.number_of_edges(), 5u); // the diagonal is shared now
}

TEST(RepairTest, BothSidesOfAZeroThicknessPanelAreKept) {
    // Front and back of the same square: same corners, opposite winding. CGAL's default
    // duplicate removal would delete one side.
    MeshData panel = Square();
    panel.indices.insert(panel.indices.end(), {0, 2, 1, 0, 3, 2});
    const RepairedMesh repaired = repair(panel);

    EXPECT_EQ(repaired.stats.removed_duplicate_triangles, 0u);
    EXPECT_EQ(repaired.mesh.number_of_faces(), 4u);
    EXPECT_EQ(CanonicalTriangles(Flatten(repaired.mesh)), CanonicalTriangles(panel));
}

TEST(RepairTest, ExactDuplicateTriangleIsRemoved) {
    MeshData mesh = Square();
    mesh.indices.insert(mesh.indices.end(), {0, 1, 2});
    const RepairedMesh repaired = repair(mesh);

    EXPECT_EQ(repaired.stats.removed_duplicate_triangles, 1u);
    EXPECT_EQ(repaired.mesh.number_of_faces(), 2u);
}

TEST(RepairTest, UnusedVerticesAreDropped) {
    MeshData mesh = Square();
    mesh.positions.insert(mesh.positions.end(), {5, 5, 5});
    const RepairedMesh repaired = repair(mesh);

    EXPECT_EQ(repaired.stats.removed_unused_vertices, 1u);
    EXPECT_EQ(repaired.mesh.number_of_vertices(), 4u);
}

TEST(RepairTest, DegenerateTrianglesAreRemoved) {
    MeshData mesh = Square();
    // A repeated corner, and three collinear corners along the square's bottom edge.
    mesh.positions.insert(mesh.positions.end(), {0.5f, 0, 0});
    mesh.indices.insert(mesh.indices.end(), {1, 1, 2, 0, 4, 1});
    const RepairedMesh repaired = repair(mesh);

    EXPECT_EQ(repaired.stats.removed_degenerate_triangles, 2u);
    EXPECT_EQ(repaired.mesh.number_of_faces(), 2u);
}

TEST(RepairTest, FinOnAPlateIsSplitNotFlipped) {
    // Two plate triangles and a fin standing on their shared edge: three triangles on one
    // edge. The fin's winding is arbitrary relative to the plate and must be kept as is.
    const MeshData mesh{"fin",
        {0, 0, 0, 1, 0, 0, 0.5f, 1, 0, 0.5f, -1, 0, 0.5f, 0, 1},
        {0, 1, 2, 1, 0, 3, 0, 1, 4}};
    const RepairedMesh repaired = repair(mesh);

    EXPECT_GT(repaired.stats.split_vertices, 0u);
    EXPECT_EQ(repaired.mesh.number_of_faces(), 3u);
    EXPECT_EQ(CanonicalTriangles(Flatten(repaired.mesh)), CanonicalTriangles(mesh));
}

TEST(RepairTest, InconsistentWindingIsRefused) {
    // The square with its second triangle flipped: both traverse the diagonal 2 -> 0.
    MeshData mesh = Square();
    mesh.indices = {0, 1, 2, 0, 3, 2};

    EXPECT_THROW(repair(mesh), std::invalid_argument);
}
