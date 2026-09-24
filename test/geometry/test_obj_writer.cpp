#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <vector>

#include "mesh_data.h"
#include "obj_writer.h"
#include "rotatable_mesh_geometry.h"
#include "static_mesh_geometry.h"
#include "test_helpers.h"

using namespace vat::geometry;

namespace {

// A per-test file in the system temp directory, removed again afterwards.
class ObjWriterTest : public ::testing::Test {
protected:
    std::filesystem::path out_path;

    void SetUp() override {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        out_path = std::filesystem::temp_directory_path()
            / (std::string("vat_obj_writer_") + info->name() + ".obj");
    }

    void TearDown() override {
        std::error_code ignored;
        std::filesystem::remove(out_path, ignored);
    }

    std::string Out() const { return out_path.string(); }
};

MeshData Triangle(std::string name, std::vector<float> positions) {
    return MeshData{std::move(name), std::move(positions), {0, 1, 2}};
}

// Distance in representable floats; 0 means bit-identical.
std::int64_t UlpDistance(float a, float b) {
    std::int32_t ia, ib;
    std::memcpy(&ia, &a, sizeof(a));
    std::memcpy(&ib, &b, sizeof(b));
    if ((ia < 0) != (ib < 0)) {
        return a == b ? 0 : INT64_MAX; // +0 vs -0 is fine, any other sign change is not
    }
    return std::llabs(static_cast<std::int64_t>(ia) - ib);
}

// What write_obj promises: the same meshes, names, triangles and winding, with every
// coordinate within one ulp. Derived quantities then agree to rounding, scaled by how
// skinny the triangles are, hence the looser tolerances there.
void ExpectRoundTripped(StaticMeshGeometry& original, StaticMeshGeometry& reloaded) {
    ASSERT_EQ(original.get_num_triangles(), reloaded.get_num_triangles());
    EXPECT_EQ(ToVector(original.get_num_triangles_per_mesh()), ToVector(reloaded.get_num_triangles_per_mesh()));
    EXPECT_EQ(ToVector(original.get_mesh_names()), ToVector(reloaded.get_mesh_names()));
    EXPECT_EQ(ToVector(original.get_triangle_ids()), ToVector(reloaded.get_triangle_ids()));

    const auto va = original.get_vertices();
    const auto vb = reloaded.get_vertices();
    std::int64_t max_ulps = 0;
    for (std::size_t i = 0; i < va.size(); ++i) {
        max_ulps = std::max(max_ulps, UlpDistance(va[i], vb[i]));
    }
    EXPECT_LE(max_ulps, 1) << "a coordinate moved by more than one ulp";

    const auto na = original.get_normals(), nb = reloaded.get_normals();
    const auto ca = original.get_centroids(), cb = reloaded.get_centroids();
    for (std::size_t i = 0; i < na.size(); ++i) {
        EXPECT_NEAR(na[i], nb[i], 1e-4f);
        EXPECT_NEAR(ca[i], cb[i], 1e-6f);
    }
    const auto aa = original.get_areas(), ab = reloaded.get_areas();
    for (std::size_t i = 0; i < aa.size(); ++i) {
        EXPECT_NEAR(aa[i], ab[i], 1e-4f * aa[i]);
    }
    EXPECT_FLOAT_EQ(original.get_bounding_sphere_radius(), reloaded.get_bounding_sphere_radius());
}

const std::array<float, 3> ORIGIN{-0.15f, 0.1f, 0.05f};
const std::array<float, 3> AXIS{0.0f, 1.0f, 0.0f};

} // namespace

TEST_F(ObjWriterTest, ShuttlecockSurvivesARoundTrip) {
    StaticMeshGeometry original(GetTestDataPath(__FILE__, "../../matlab/examples/geometries/shuttlecock_960.obj"));
    write_obj(original, Out());
    StaticMeshGeometry reloaded(Out());

    ExpectRoundTripped(original, reloaded);
}

TEST_F(ObjWriterTest, OpenMeshWithDuplicateVerticesSurvivesARoundTrip) {
    // soar_satellite has border edges and position-duplicated vertices, and its source
    // file splits meshes with materials; none of that may change what comes back.
    StaticMeshGeometry original(GetTestDataPath(__FILE__, "../../matlab/examples/geometries/soar_satellite.obj"));
    write_obj(original, Out());
    StaticMeshGeometry reloaded(Out());

    ExpectRoundTripped(original, reloaded);
}

TEST_F(ObjWriterTest, AwkwardCoordinatesReadBackWithinOneUlp) {
    // Values with no short decimal form, a wide range of magnitudes, and 5e-4, whose
    // shortest decimal spelling assimp reads back one ulp low.
    const std::vector<float> positions = {
        0.1f, 1.0f / 3.0f, -2.0f / 7.0f,
        1.0e-7f, 123456.789f, -0.000123456f,
        3.14159265f, -1.0e6f, 5.0e-4f,
    };
    StaticMeshGeometry original(std::vector<MeshData>{Triangle("tri", positions)});
    write_obj(original, Out());
    StaticMeshGeometry reloaded(Out());

    ExpectRoundTripped(original, reloaded);
}

TEST_F(ObjWriterTest, MeshOrderAndNamesArePreserved) {
    std::vector<MeshData> meshes{
        Triangle("zeta", {0, 0, 0, 1, 0, 0, 0, 1, 0}),
        Triangle("alpha", {0, 0, 1, 1, 0, 1, 0, 1, 1}),
        Triangle("", {0, 0, 2, 1, 0, 2, 0, 1, 2}),
    };
    StaticMeshGeometry original(std::move(meshes));
    write_obj(original, Out());
    StaticMeshGeometry reloaded(Out());

    // "Mesh_2" is the default a nameless mesh gets.
    const std::vector<std::string> expected{"zeta", "alpha", "Mesh_2"};
    EXPECT_EQ(ToVector(original.get_mesh_names()), expected);
    EXPECT_EQ(ToVector(reloaded.get_mesh_names()), expected);
}

TEST_F(ObjWriterTest, WhitespaceInNamesBecomesUnderscores) {
    StaticMeshGeometry original(std::vector<MeshData>{
        Triangle("solar array\tleft", {0, 0, 0, 1, 0, 0, 0, 1, 0}),
    });
    write_obj(original, Out());
    StaticMeshGeometry reloaded(Out());

    EXPECT_EQ(reloaded.get_mesh_names()[0], "solar_array_left");
}

TEST_F(ObjWriterTest, TurnedGeometryIsRefused) {
    RotatableMeshGeometry geometry(GetTestDataPath(__FILE__, "../../matlab/examples/geometries/shuttlecock_960.obj"));
    geometry.turn_mesh_around_axis(1, 0.3f, ORIGIN, AXIS);

    EXPECT_THROW(write_obj(geometry, Out()), std::invalid_argument);
    EXPECT_FALSE(std::filesystem::exists(out_path));
}

TEST_F(ObjWriterTest, GeometryTurnedBackToZeroCanBeWritten) {
    RotatableMeshGeometry geometry(GetTestDataPath(__FILE__, "../../matlab/examples/geometries/shuttlecock_960.obj"));
    geometry.turn_mesh_around_axis(1, 0.3f, ORIGIN, AXIS);
    geometry.turn_mesh_around_axis(1, 0.0f, ORIGIN, AXIS);

    EXPECT_NO_THROW(write_obj(geometry, Out()));
}

TEST_F(ObjWriterTest, MeshWithoutTrianglesIsRefused) {
    std::vector<MeshData> meshes{
        Triangle("kept", {0, 0, 0, 1, 0, 0, 0, 1, 0}),
        MeshData{"empty", {0, 0, 0}, {}},
    };
    StaticMeshGeometry geometry(std::move(meshes));

    EXPECT_THROW(write_obj(geometry, Out()), std::invalid_argument);
}

TEST_F(ObjWriterTest, EmptyGeometryIsRefused) {
    StaticMeshGeometry geometry(std::vector<MeshData>{});

    EXPECT_THROW(write_obj(geometry, Out()), std::invalid_argument);
}

TEST_F(ObjWriterTest, UnwritablePathIsReported) {
    StaticMeshGeometry geometry(std::vector<MeshData>{Triangle("tri", {0, 0, 0, 1, 0, 0, 0, 1, 0})});
    const auto missing_dir = std::filesystem::temp_directory_path() / "vat_no_such_dir" / "x.obj";

    EXPECT_THROW(write_obj(geometry, missing_dir.string()), std::runtime_error);
}
