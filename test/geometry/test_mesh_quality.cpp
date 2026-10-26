#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "mesh_data.h"
#include "mesh_quality.h"
#include "rotatable_mesh_geometry.h"
#include "static_mesh_geometry.h"
#include "test_helpers.h"

using namespace vat::geometry;

namespace {

MeshQuality QualityOf(std::vector<float> positions, std::vector<std::uint32_t> indices) {
    StaticMeshGeometry geometry(std::vector<MeshData>{MeshData{"m", std::move(positions), std::move(indices)}});
    return compute_mesh_quality(geometry);
}

} // namespace

TEST(MeshQualityTest, EquilateralTriangleIsIdeal) {
    const float h = std::sqrt(3.0f) / 2.0f;
    const MeshQuality q = QualityOf({0, 0, 0, 1, 0, 0, 0.5f, h, 0}, {0, 1, 2});

    EXPECT_EQ(q.num_triangles, 1u);
    EXPECT_EQ(q.num_degenerate, 0u);
    EXPECT_NEAR(q.aspect_ratio.median, 1.0f, 1e-6f);
    EXPECT_NEAR(q.total_area__m2, std::sqrt(3.0f) / 4.0f, 1e-6f);
    EXPECT_NEAR(q.min_altitude__m.median, h, 1e-6f);
}

TEST(MeshQualityTest, TetrahedronHasKnownValues) {
    // Two faces are equilateral with edge sqrt(0.5); two are right isosceles with legs
    // sqrt(0.5) and hypotenuse 1.
    StaticMeshGeometry geometry(GetTestDataPath(__FILE__, "../geometries/tetraeder.obj"));
    const MeshQuality q = compute_mesh_quality(geometry);

    const double equilateral_area = std::sqrt(3.0) / 4.0 * 0.5;
    const double right_area = 0.25;
    const double right_inradius = right_area / ((1.0 + 2.0 * std::sqrt(0.5)) / 2.0);
    const double right_aspect = 1.0 / (2.0 * std::sqrt(3.0) * right_inradius);

    EXPECT_EQ(q.num_triangles, 4u);
    EXPECT_NEAR(q.total_area__m2, 2 * equilateral_area + 2 * right_area, 1e-6);
    EXPECT_NEAR(q.mean_area__m2, (2 * equilateral_area + 2 * right_area) / 4, 1e-6);
    EXPECT_NEAR(q.aspect_ratio.min, 1.0, 1e-6);
    EXPECT_NEAR(q.aspect_ratio.max, right_aspect, 1e-5);
    EXPECT_NEAR(q.min_altitude__m.min, 0.5, 1e-6);                                // right: 2A / 1
    EXPECT_NEAR(q.min_altitude__m.max, 2 * equilateral_area / std::sqrt(0.5), 1e-6); // equilateral
    EXPECT_NEAR(q.bounding_sphere_radius__m, 0.5f, 1e-7f);
}

TEST(MeshQualityTest, SliverIsNarrowAndHasLargeAspectRatio) {
    // Base 1, apex 0.01 above its midpoint: area 0.005, narrowest width 0.01.
    const MeshQuality q = QualityOf({0, 0, 0, 1, 0, 0, 0.5f, 0.01f, 0}, {0, 1, 2});

    EXPECT_NEAR(q.min_altitude__m.median, 0.01f, 1e-7f);
    EXPECT_NEAR(q.area__m2.median, 0.005f, 1e-8f);
    EXPECT_NEAR(q.aspect_ratio.median, 57.7, 0.1);
}

TEST(MeshQualityTest, DegenerateTrianglesAreCountedNotRanked) {
    // One proper triangle and one whose corners are collinear.
    const MeshQuality q = QualityOf(
        {0, 0, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0},
        {0, 1, 2, 0, 1, 3});

    EXPECT_EQ(q.num_triangles, 2u);
    EXPECT_EQ(q.num_degenerate, 1u);
    EXPECT_EQ(q.area__m2.min, 0.0f);
    EXPECT_EQ(q.min_altitude__m.min, 0.0f);
    // The aspect ratio of the degenerate one is infinite and left out.
    EXPECT_TRUE(std::isfinite(q.aspect_ratio.max));
}

TEST(MeshQualityTest, ShuttlecockMatchesAnIndependentComputation) {
    // Reference values computed separately in Python from the .obj, same percentile
    // convention. Median aspect ratio ~14 and mean/median area ~15 are the defects that
    // motivate remeshing in the first place.
    StaticMeshGeometry geometry(GetTestDataPath(__FILE__, "../../matlab/examples/geometries/shuttlecock_960.obj"));
    const MeshQuality q = compute_mesh_quality(geometry);

    EXPECT_EQ(q.num_triangles, 960u);
    EXPECT_NEAR(q.total_area__m2, 0.274726003, 1e-6);
    EXPECT_NEAR(q.mean_area__m2, 0.00028617292, 1e-9);
    EXPECT_NEAR(q.area__m2.median, 1.87734653e-05, 1e-10);
    EXPECT_NEAR(q.aspect_ratio.min, 1.39384685, 1e-5);
    EXPECT_NEAR(q.aspect_ratio.median, 13.778974, 1e-4);
    EXPECT_NEAR(q.aspect_ratio.p95, 38.8333945, 1e-3);
    EXPECT_NEAR(q.min_altitude__m.p05, 0.000749313688, 1e-9);
    EXPECT_NEAR(q.min_altitude__m.median, 0.000749917369, 1e-9);
    EXPECT_NEAR(q.bounding_sphere_radius__m, 0.358237337, 1e-6);
}

TEST(MeshQualityTest, TurningAMeshKeepsTriangleShapes) {
    const std::string path = GetTestDataPath(__FILE__, "../../matlab/examples/geometries/shuttlecock_960.obj");
    RotatableMeshGeometry geometry(path);
    const MeshQuality before = compute_mesh_quality(geometry);
    geometry.turn_mesh_around_axis(1, 1.2f, std::array<float, 3>{-0.15f, 0.1f, 0.05f}, std::array<float, 3>{0.0f, 1.0f, 0.0f});
    const MeshQuality after = compute_mesh_quality(geometry);

    EXPECT_NEAR(after.total_area__m2, before.total_area__m2, 1e-6f * before.total_area__m2);
    EXPECT_NEAR(after.aspect_ratio.median, before.aspect_ratio.median, 1e-3f);
    EXPECT_NEAR(after.min_altitude__m.median, before.min_altitude__m.median, 1e-7f);
    // Whatever the pose, the radius reported is the one the shading pass will use now.
    EXPECT_EQ(after.bounding_sphere_radius__m, geometry.get_bounding_sphere_radius());
}

TEST(MeshQualityTest, EmptyGeometryIsRejected) {
    StaticMeshGeometry geometry(std::vector<MeshData>{});
    EXPECT_THROW(compute_mesh_quality(geometry), std::invalid_argument);
}
