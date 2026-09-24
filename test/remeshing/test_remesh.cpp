#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#include <glm/glm.hpp>

#include "core.h"
#include "mesh_data.h"
#include "remesh.h"
#include "rotatable_mesh_geometry.h"
#include "sentman.h"
#include "static_mesh_geometry.h"
#include "test_helpers.h"

using namespace vat::remeshing;
using vat::geometry::MeshData;
using vat::geometry::RotatableMeshGeometry;
using vat::geometry::StaticMeshGeometry;

namespace {

std::string Shuttlecock() {
    return GetTestDataPath(__FILE__, "../../matlab/examples/geometries/shuttlecock_960.obj");
}

// A square panel of side 1 m in z = 0, cut into just two triangles.
StaticMeshGeometry SquarePanel() {
    return StaticMeshGeometry(std::vector<MeshData>{
        MeshData{"panel", {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0}, {0, 1, 2, 0, 2, 3}}});
}

struct Load {
    glm::vec3 force__N{0.0f};
    glm::vec3 torque__Nm{0.0f};
};

// Sentman force and torque summed over every triangle, with no shading. Per triangle both
// are linear in its area (torque through area times centroid), so on a flat facet the
// sums do not depend on how the facet is cut into triangles. Any flipped triangle, lost
// area or moved rim shows up here.
Load UnshadedSentmanLoad(StaticMeshGeometry& geometry, const glm::vec3& v_rel__m_per_s) {
    vat::gsi_models::Sentman sentman(1, 0.9f);
    vat::AeroConditions aero{1.2482e-11f, 934.0f, 16 * 1.6605390689252e-27f};
    const auto areas = geometry.get_areas();
    const auto normals = geometry.get_normals();
    const auto centroids = geometry.get_centroids();
    // Accumulated in double: tens of thousands of float terms would otherwise dominate
    // the comparison.
    glm::dvec3 force(0.0), torque(0.0);
    for (unsigned int t = 0; t < geometry.get_num_triangles(); ++t) {
        glm::vec3 f, m;
        sentman.calc_aero_force_and_torque(areas[t],
            glm::vec3(normals[3 * t], normals[3 * t + 1], normals[3 * t + 2]),
            glm::vec3(centroids[3 * t], centroids[3 * t + 1], centroids[3 * t + 2]),
            v_rel__m_per_s, 300.0f, aero, f, m);
        force += glm::dvec3(f);
        torque += glm::dvec3(m);
    }
    return Load{glm::vec3(force), glm::vec3(torque)};
}

void ExpectNearVec(const glm::vec3& actual, const glm::vec3& expected, float tolerance, const char* what) {
    EXPECT_LE(glm::length(actual - expected), tolerance)
        << what << ": (" << actual.x << ", " << actual.y << ", " << actual.z << ") vs ("
        << expected.x << ", " << expected.y << ", " << expected.z << ")";
}

} // namespace

TEST(RemeshTest, FlatPanelBecomesNearlyEquilateral) {
    StaticMeshGeometry panel = SquarePanel();
    RemeshOptions options;
    options.target_edge_length__m = 0.05f;
    const RemeshResult result = remesh(panel, options);
    const RemeshReport& r = result.report;

    EXPECT_NEAR(r.target_edge_length__m, 0.05f, 1e-7f);
    // Two triangles on a square split into a regular grid of right isosceles triangles
    // (aspect ratio 1.39) in which every vertex already has valence 6 and sits at its
    // neighbours' centroid, so neither flips nor relaxation move it towards equilateral.
    // That is a stable state of the algorithm, and nowhere near a sliver.
    EXPECT_LT(r.after.aspect_ratio.max, 1.5f);
    // A flat panel keeps its area exactly, and its rim stays where it was.
    EXPECT_NEAR(r.area_change__percent, 0.0f, 1e-4f);
    EXPECT_FLOAT_EQ(r.after.bounding_sphere_radius__m, r.before.bounding_sphere_radius__m);
    // 1 m^2 / (sqrt(3)/4 * 0.05^2) = 924 ideal equilateral triangles.
    EXPECT_NEAR(static_cast<double>(r.after.num_triangles), 924.0, 0.25 * 924.0);
}

TEST(RemeshTest, TriangleCountTargetLandsNearIt) {
    StaticMeshGeometry geometry(Shuttlecock());
    RemeshOptions options;
    options.target_triangle_count = 20000;
    const RemeshResult result = remesh(geometry, options);

    EXPECT_NEAR(static_cast<double>(result.report.after.num_triangles), 20000.0, 0.25 * 20000.0);
    EXPECT_NEAR(result.report.target_edge_length__m,
        std::sqrt(result.report.before.total_area__m2 / (0.4330127f * 20000.0f)), 1e-6f);
}

TEST(RemeshTest, ShuttlecockKeepsItsPartsAndLosesItsSlivers) {
    StaticMeshGeometry geometry(Shuttlecock());
    RemeshOptions options;
    options.target_triangle_count = 20000;
    const RemeshResult result = remesh(geometry, options);
    RotatableMeshGeometry& remeshed = *result.geometry;
    const RemeshReport& r = result.report;

    // Same parts under the same mesh_ids.
    EXPECT_EQ(ToVector(remeshed.get_mesh_names()), ToVector(geometry.get_mesh_names()));
    ASSERT_EQ(r.triangles_per_mesh_after.size(), r.triangles_per_mesh_before.size());
    for (unsigned int count : r.triangles_per_mesh_after) {
        EXPECT_GT(count, 0u);
    }

    // The shuttlecock is all flat faces, so its area is kept, and its extent with it.
    EXPECT_NEAR(r.area_change__percent, 0.0f, 1e-3f);
    EXPECT_FLOAT_EQ(r.after.bounding_sphere_radius__m, r.before.bounding_sphere_radius__m);

    // The point of it all: median aspect ratio ~13.8 before, near-equilateral after.
    EXPECT_GT(r.before.aspect_ratio.median, 10.0f);
    EXPECT_LT(r.after.aspect_ratio.median, 1.5f);
    EXPECT_EQ(r.after.num_degenerate, 0u);
}

TEST(RemeshTest, UnshadedLoadsAreUnchangedOnFlatGeometry) {
    StaticMeshGeometry geometry(Shuttlecock());
    RemeshOptions options;
    options.target_triangle_count = 20000;
    RemeshResult result = remesh(geometry, options);

    const std::array<glm::vec3, 4> flows{
        glm::vec3(7800.0f, 0.0f, 0.0f),
        glm::vec3(7000.0f, 3000.0f, 1500.0f),
        glm::vec3(-3000.0f, 1000.0f, 7000.0f),
        glm::vec3(0.0f, -7500.0f, 2000.0f),
    };
    for (const glm::vec3& v_rel : flows) {
        const Load before = UnshadedSentmanLoad(geometry, v_rel);
        const Load after = UnshadedSentmanLoad(*result.geometry, v_rel);
        ExpectNearVec(after.force__N, before.force__N, 1e-4f * glm::length(before.force__N), "force");
        ExpectNearVec(after.torque__Nm, before.torque__Nm, 1e-4f * glm::length(before.force__N) * 0.36f, "torque");
    }
}

TEST(RemeshTest, OpenPanelsKeepTheirRims) {
    StaticMeshGeometry geometry(GetTestDataPath(__FILE__, "../../matlab/examples/geometries/soar_satellite.obj"));
    RemeshOptions options;
    options.target_triangle_count = 8000;
    RemeshResult result = remesh(geometry, options);
    const RemeshReport& r = result.report;

    EXPECT_EQ(ToVector(result.geometry->get_mesh_names()), ToVector(geometry.get_mesh_names()));
    EXPECT_GT(r.repair.split_vertices, 0u);
    EXPECT_NEAR(r.area_change__percent, 0.0f, 0.5f);
    EXPECT_FLOAT_EQ(r.after.bounding_sphere_radius__m, r.before.bounding_sphere_radius__m);
    EXPECT_LT(r.after.aspect_ratio.median, 1.5f);
}

TEST(RemeshTest, InputIsLeftUntouched) {
    StaticMeshGeometry geometry(Shuttlecock());
    StaticMeshGeometry reference(Shuttlecock());
    RemeshOptions options;
    options.target_triangle_count = 5000;
    remesh(geometry, options);

    ExpectSameGeometry(geometry, reference);
}

TEST(RemeshTest, TurnedInputIsRemeshedUnturned) {
    RotatableMeshGeometry turned(Shuttlecock());
    turned.turn_mesh_around_axis(1, 0.8f, std::array<float, 3>{-0.15f, 0.1f, 0.05f},
        std::array<float, 3>{0.0f, 1.0f, 0.0f});
    StaticMeshGeometry unturned(Shuttlecock());
    RemeshOptions options;
    options.target_triangle_count = 5000;

    const RemeshResult from_turned = remesh(turned, options);
    const RemeshResult from_unturned = remesh(unturned, options);

    for (const glm::mat4& m : from_turned.geometry->get_model_matrices()) {
        EXPECT_EQ(m, glm::mat4(1.0f));
    }
    ExpectSameGeometry(*from_turned.geometry, *from_unturned.geometry);
}

TEST(RemeshTest, SizeMustBeGivenExactlyOnce) {
    StaticMeshGeometry panel = SquarePanel();
    RemeshOptions neither;
    EXPECT_THROW(remesh(panel, neither), std::invalid_argument);

    RemeshOptions both;
    both.target_triangle_count = 100;
    both.target_edge_length__m = 0.1f;
    EXPECT_THROW(remesh(panel, both), std::invalid_argument);

    RemeshOptions negative;
    negative.target_edge_length__m = -0.1f;
    EXPECT_THROW(remesh(panel, negative), std::invalid_argument);

    RemeshOptions no_iterations;
    no_iterations.target_edge_length__m = 0.1f;
    no_iterations.iterations = 0;
    EXPECT_THROW(remesh(panel, no_iterations), std::invalid_argument);
}

TEST(RemeshTest, EmptyGeometryIsRejected) {
    StaticMeshGeometry empty(std::vector<MeshData>{});
    RemeshOptions options;
    options.target_triangle_count = 100;
    EXPECT_THROW(remesh(empty, options), std::invalid_argument);
}

TEST(RemeshTest, UnrepairableMeshIsRejected) {
    // A square whose second triangle is wound against the first.
    StaticMeshGeometry flipped(std::vector<MeshData>{
        MeshData{"flipped", {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0}, {0, 1, 2, 0, 3, 2}}});
    RemeshOptions options;
    options.target_triangle_count = 100;
    EXPECT_THROW(remesh(flipped, options), std::invalid_argument);
}

TEST(RemeshPredictionTest, EdgeLengthGivesTheIdealTilingCount) {
    StaticMeshGeometry panel = SquarePanel();
    RemeshOptions options;
    options.target_edge_length__m = 0.05f;
    const RemeshPrediction p = predict_remesh(panel, options);

    // 1 m^2 / (sqrt(3)/4 * 0.05^2) = 923.8
    EXPECT_EQ(p.predicted_triangles, 924u);
    EXPECT_FLOAT_EQ(p.target_edge_length__m, 0.05f);
    EXPECT_FLOAT_EQ(p.total_area__m2, 1.0f);
    EXPECT_EQ(p.predicted_memory__bytes, 924u * 154u);
}

TEST(RemeshPredictionTest, TriangleCountIsPredictedAsGiven) {
    StaticMeshGeometry geometry(Shuttlecock());
    RemeshOptions options;
    options.target_triangle_count = 20000;
    const RemeshPrediction p = predict_remesh(geometry, options);

    EXPECT_EQ(p.predicted_triangles, 20000u);
    EXPECT_NEAR(p.total_area__m2, 0.274726f, 1e-6f);
}

TEST(RemeshPredictionTest, PredictionUsesTheSameEdgeLengthAsRemesh) {
    StaticMeshGeometry geometry(Shuttlecock());
    RemeshOptions options;
    options.target_triangle_count = 3000;

    EXPECT_EQ(predict_remesh(geometry, options).target_edge_length__m,
        remesh(geometry, options).report.target_edge_length__m);
}

TEST(RemeshPredictionTest, OversizedRequestIsReportedByPredictionAndRefusedByRemesh) {
    StaticMeshGeometry geometry(Shuttlecock());

    RemeshOptions too_many;
    too_many.target_triangle_count = REMESH_MAX_TRIANGLES + 1;
    EXPECT_EQ(predict_remesh(geometry, too_many).predicted_triangles, REMESH_MAX_TRIANGLES + 1);
    EXPECT_THROW(remesh(geometry, too_many), std::invalid_argument);

    // 0.2 mm edges on the shuttlecock: the 2.2-million-triangle case.
    RemeshOptions too_fine;
    too_fine.target_edge_length__m = 0.0002f;
    EXPECT_GT(predict_remesh(geometry, too_fine).predicted_triangles, 1'500'000u);
    EXPECT_THROW(remesh(geometry, too_fine), std::invalid_argument);
}

TEST(RemeshPredictionTest, AbsurdlySmallEdgeSaturatesInsteadOfOverflowing) {
    StaticMeshGeometry geometry(Shuttlecock());
    RemeshOptions options;
    options.target_edge_length__m = 1e-9f;

    EXPECT_EQ(predict_remesh(geometry, options).predicted_triangles, std::numeric_limits<unsigned int>::max());
    EXPECT_THROW(remesh(geometry, options), std::invalid_argument);
}
