#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "mesh_data.h"
#include "rotatable_mesh_geometry.h"
#include "static_mesh_geometry.h"
#include "test_helpers.h"

using namespace vat::geometry;

namespace {

std::string TetraPath() {
    return GetTestDataPath(__FILE__, "../geometries/tetraeder.obj");
}

// test/geometries/tetraeder.obj written out by hand: same vertices, faces 1-based there.
MeshData HandBuiltTetra(std::string name = "") {
    return MeshData{
        std::move(name),
        {0.0f, -0.5f, 0.0f,  0.0f, 0.5f, 0.0f,  0.0f, 0.0f, 0.5f,  -0.5f, 0.0f, 0.0f},
        {0, 3, 1,  1, 3, 2,  2, 3, 0,  0, 1, 2},
    };
}

template <typename T>
std::vector<T> ToVector(std::span<const T> s) {
    return std::vector<T>(s.begin(), s.end());
}

void ExpectSameGeometry(StaticMeshGeometry& a, StaticMeshGeometry& b) {
    EXPECT_EQ(a.get_num_triangles(), b.get_num_triangles());
    EXPECT_EQ(ToVector(a.get_vertices()), ToVector(b.get_vertices()));
    EXPECT_EQ(ToVector(a.get_triangle_ids()), ToVector(b.get_triangle_ids()));
    EXPECT_EQ(ToVector(a.get_normals()), ToVector(b.get_normals()));
    EXPECT_EQ(ToVector(a.get_centroids()), ToVector(b.get_centroids()));
    EXPECT_EQ(ToVector(a.get_areas()), ToVector(b.get_areas()));
    EXPECT_EQ(ToVector(a.get_num_triangles_per_mesh()), ToVector(b.get_num_triangles_per_mesh()));
    EXPECT_EQ(ToVector(a.get_mesh_names()), ToVector(b.get_mesh_names()));
    EXPECT_EQ(a.get_bounding_sphere_radius(), b.get_bounding_sphere_radius());
}

} // namespace

TEST(MeshDataTest, LoaderKeepsIndexedConnectivity) {
    const std::vector<MeshData> meshes = load_mesh_data(TetraPath());
    ASSERT_EQ(meshes.size(), 1u);

    // Shared corners stay shared: 4 vertices, not one per triangle corner.
    EXPECT_EQ(meshes[0].positions.size(), 3u * 4u);

    // Assimp numbers vertices in first-use order rather than the file's `v` order, so
    // compare the triangles themselves: same order, same corners, same winding.
    auto corners = [](const MeshData& m) {
        std::vector<float> out;
        for (const std::uint32_t i : m.indices) {
            out.insert(out.end(), m.positions.begin() + 3 * i, m.positions.begin() + 3 * i + 3);
        }
        return out;
    };
    EXPECT_EQ(corners(meshes[0]), corners(HandBuiltTetra()));
}

TEST(MeshDataTest, MissingFileGivesNoMeshes) {
    EXPECT_TRUE(load_mesh_data(GetTestDataPath(__FILE__, "does_not_exist.obj")).empty());
}

TEST(MeshDataTest, ArrayConstructionMatchesFileConstructionExactly) {
    StaticMeshGeometry from_file(TetraPath());
    std::vector<MeshData> meshes;
    meshes.push_back(HandBuiltTetra(from_file.get_mesh_names()[0]));
    StaticMeshGeometry from_arrays(std::move(meshes));

    ExpectSameGeometry(from_file, from_arrays);
}

TEST(MeshDataTest, GeometryExposesTheMeshesItWasBuiltFrom) {
    const std::string path = GetTestDataPath(__FILE__, "../../examples/geometry_files/shuttlecock_15k.obj");
    StaticMeshGeometry geometry(path);
    const std::vector<MeshData> loaded = load_mesh_data(path);

    const auto kept = geometry.get_mesh_data();
    ASSERT_EQ(kept.size(), loaded.size());
    for (std::size_t i = 0; i < kept.size(); ++i) {
        EXPECT_EQ(kept[i].name, geometry.get_mesh_names()[i]);
        EXPECT_EQ(kept[i].positions, loaded[i].positions);
        EXPECT_EQ(kept[i].indices, loaded[i].indices);
        EXPECT_EQ(kept[i].indices.size() / 3, geometry.get_num_triangles_per_mesh()[i]);
    }

    // Rebuilding from what the geometry kept must give the geometry back.
    StaticMeshGeometry rebuilt(std::vector<MeshData>(kept.begin(), kept.end()));
    ExpectSameGeometry(geometry, rebuilt);
}

TEST(MeshDataTest, UnnamedMeshesGetTheirIndexAsName) {
    std::vector<MeshData> meshes{HandBuiltTetra(), HandBuiltTetra("named")};
    StaticMeshGeometry geometry(std::move(meshes));

    EXPECT_EQ(geometry.get_mesh_names()[0], "Mesh 0");
    EXPECT_EQ(geometry.get_mesh_names()[1], "named");
    EXPECT_EQ(geometry.get_mesh_data()[0].name, "Mesh 0");
}

TEST(MeshDataTest, MalformedMeshesAreRejected) {
    MeshData ragged = HandBuiltTetra();
    ragged.positions.pop_back();
    EXPECT_THROW(StaticMeshGeometry(std::vector<MeshData>{ragged}), std::invalid_argument);

    MeshData out_of_range = HandBuiltTetra();
    out_of_range.indices.back() = 4;
    EXPECT_THROW(StaticMeshGeometry(std::vector<MeshData>{out_of_range}), std::invalid_argument);
}

TEST(MeshDataTest, RotatableGeometryFromArraysTurnsLikeOneFromFile) {
    RotatableMeshGeometry from_file(TetraPath());
    RotatableMeshGeometry from_arrays(std::vector<MeshData>{HandBuiltTetra()});

    const float angle__rad = 0.7f;
    const std::array<float, 3> origin{0.1f, 0.0f, 0.0f};
    const std::array<float, 3> axis{0.0f, 0.0f, 1.0f};
    from_file.turn_mesh_around_axis(0, angle__rad, origin, axis);
    from_arrays.turn_mesh_around_axis(0, angle__rad, origin, axis);

    EXPECT_EQ(ToVector(from_file.get_vertices()), ToVector(from_arrays.get_vertices()));
    EXPECT_EQ(ToVector(from_file.get_normals()), ToVector(from_arrays.get_normals()));
    EXPECT_EQ(ToVector(from_file.get_centroids()), ToVector(from_arrays.get_centroids()));
    // The indexed source data is the untransformed mesh, whatever the pose.
    EXPECT_EQ(from_arrays.get_mesh_data()[0].positions, HandBuiltTetra().positions);
}
