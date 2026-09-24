#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <vector>

#include "mesh_data.h"
#include "pixel_sizing.h"
#include "shading_pipeline.h"
#include "static_mesh_geometry.h"

using namespace vat::shading;
using vat::geometry::MeshData;
using vat::geometry::StaticMeshGeometry;

namespace {

std::string DataPath(const std::string& relative) {
    return (std::filesystem::path(__FILE__).parent_path() / relative).string();
}

// One triangle of the given narrowest width, with a far vertex fixing R = 1 m.
StaticMeshGeometry ThinTriangle(float width__m) {
    return StaticMeshGeometry(std::vector<MeshData>{
        MeshData{"thin", {-0.5f, 0, 0, 0.5f, 0, 0, 0, width__m, 0, 0, 0, 1.0f}, {0, 1, 2, 0, 3, 1}}});
}

} // namespace

TEST(PixelSizingTest, NumPixelPutsThreePixelsAcrossTheWidth) {
    // Binary-exact values: 2 * 0.5 * 3 / 0.25 = 12 and 2 * 0.5 * 3 / 2^-7 = 384.
    EXPECT_EQ(num_pixel_for_triangle_width(0.5f, 0.25f), 12u);
    EXPECT_EQ(num_pixel_for_triangle_width(0.5f, 0.0078125f), 384u);
    // Rounds up, so the width is never under-resolved.
    EXPECT_EQ(num_pixel_for_triangle_width(0.5f, 0.26f), 12u);
    EXPECT_EQ(num_pixel_for_triangle_width(0.5f, 0.24f), 13u);
}

TEST(PixelSizingTest, InvalidLengthsAreRejected) {
    EXPECT_THROW(num_pixel_for_triangle_width(0.0f, 0.01f), std::invalid_argument);
    EXPECT_THROW(num_pixel_for_triangle_width(0.5f, 0.0f), std::invalid_argument);
    EXPECT_THROW(num_pixel_for_triangle_width(0.5f, -0.01f), std::invalid_argument);
    EXPECT_THROW(num_pixel_for_triangle_width(NAN, 0.01f), std::invalid_argument);
}

TEST(PixelSizingTest, SliverMeshIsSizedForItsNarrowSide) {
    // shuttlecock_960: R = 0.3582 m, 5th-percentile width 0.749 mm -> 2*0.3582*3/0.000749.
    StaticMeshGeometry geometry(DataPath("../../matlab/examples/geometries/shuttlecock_960.obj"));
    EXPECT_EQ(suggest_num_pixel(geometry), 2869u);
}

TEST(PixelSizingTest, CoarseMeshGetsTheMinimum) {
    StaticMeshGeometry tetra(DataPath("../geometries/tetraeder.obj"));
    EXPECT_EQ(suggest_num_pixel(tetra), MIN_AUTO_NUM_PIXEL);
}

TEST(PixelSizingTest, HopelesslyThinMeshIsCapped) {
    StaticMeshGeometry thin = ThinTriangle(1e-6f);
    EXPECT_EQ(suggest_num_pixel(thin), MAX_AUTO_NUM_PIXEL);

    StaticMeshGeometry degenerate = ThinTriangle(0.0f);
    EXPECT_EQ(suggest_num_pixel(degenerate), MAX_AUTO_NUM_PIXEL);
}

TEST(PixelSizingTest, PipelineWithoutNumPixelUsesTheSuggestion) {
    StaticMeshGeometry geometry(DataPath("../../matlab/examples/geometries/shuttlecock_960.obj"));
    ShadingPipeline pipeline(geometry, ShadingAlgorithmType::CoP);

    EXPECT_EQ(pipeline.get_num_pixel(), suggest_num_pixel(geometry));
    EXPECT_EQ(pipeline.shade(glm::vec3(1.0f, 0.0f, 0.0f)).size(), geometry.get_num_triangles());
}

TEST(PixelSizingTest, PipelineRejectsZeroPixels) {
    StaticMeshGeometry tetra(DataPath("../geometries/tetraeder.obj"));
    EXPECT_THROW(ShadingPipeline(tetra, ShadingAlgorithmType::Binary, 0), std::invalid_argument);
}
