#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
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

TEST(PixelSizingTest, NumPixelPutsKPixelsAcrossTheWidth) {
    EXPECT_EQ(pixels_per_triangle_width(ShadingAlgorithmType::Binary), 3.0f);
    EXPECT_EQ(pixels_per_triangle_width(ShadingAlgorithmType::CoP), 7.0f);
    // Binary-exact values: 2 * 0.5 * k / 0.25 and 2 * 0.5 * k / 2^-7.
    EXPECT_EQ(num_pixel_for_triangle_width(0.5f, 0.25f, ShadingAlgorithmType::Binary), 12u);
    EXPECT_EQ(num_pixel_for_triangle_width(0.5f, 0.25f, ShadingAlgorithmType::CoP), 28u);
    EXPECT_EQ(num_pixel_for_triangle_width(0.5f, 0.0078125f, ShadingAlgorithmType::Binary), 384u);
    // Rounds up, so the width is never under-resolved.
    EXPECT_EQ(num_pixel_for_triangle_width(0.5f, 0.26f, ShadingAlgorithmType::Binary), 12u);
    EXPECT_EQ(num_pixel_for_triangle_width(0.5f, 0.24f, ShadingAlgorithmType::Binary), 13u);
}

TEST(PixelSizingTest, InvalidLengthsAreRejected) {
    const auto cop = ShadingAlgorithmType::CoP;
    EXPECT_THROW(num_pixel_for_triangle_width(0.0f, 0.01f, cop), std::invalid_argument);
    EXPECT_THROW(num_pixel_for_triangle_width(0.5f, 0.0f, cop), std::invalid_argument);
    EXPECT_THROW(num_pixel_for_triangle_width(0.5f, -0.01f, cop), std::invalid_argument);
    EXPECT_THROW(num_pixel_for_triangle_width(NAN, 0.01f, cop), std::invalid_argument);
}

TEST(PixelSizingTest, SliverMeshIsSizedForItsNarrowSide) {
    // shuttlecock_960: R = 0.3582 m, 5th-percentile width 0.749 mm -> 2*0.3582*k/0.000749.
    StaticMeshGeometry geometry(DataPath("../../matlab/examples/geometries/shuttlecock_960.obj"));
    EXPECT_EQ(suggest_num_pixel(geometry, ShadingAlgorithmType::Binary), 2869u);
    EXPECT_EQ(suggest_num_pixel(geometry, ShadingAlgorithmType::CoP), 6694u);
}

TEST(PixelSizingTest, CoarseMeshGetsTheMinimum) {
    StaticMeshGeometry tetra(DataPath("../geometries/tetraeder.obj"));
    EXPECT_EQ(suggest_num_pixel(tetra, ShadingAlgorithmType::CoP), MIN_AUTO_NUM_PIXEL);
}

TEST(PixelSizingTest, HopelesslyThinMeshIsCapped) {
    StaticMeshGeometry thin = ThinTriangle(1e-6f);
    EXPECT_EQ(suggest_num_pixel(thin, ShadingAlgorithmType::Binary), MAX_AUTO_NUM_PIXEL);

    StaticMeshGeometry degenerate = ThinTriangle(0.0f);
    EXPECT_EQ(suggest_num_pixel(degenerate, ShadingAlgorithmType::Binary), MAX_AUTO_NUM_PIXEL);
}

TEST(PixelSizingTest, PipelineWithoutNumPixelUsesTheSuggestion) {
    StaticMeshGeometry geometry(DataPath("../../matlab/examples/geometries/shuttlecock_960.obj"));
    ShadingPipeline pipeline(geometry, ShadingAlgorithmType::CoP);

    EXPECT_EQ(pipeline.get_num_pixel(), suggest_num_pixel(geometry, ShadingAlgorithmType::CoP));
    EXPECT_EQ(pipeline.shade(glm::vec3(1.0f, 0.0f, 0.0f)).size(), geometry.get_num_triangles());
}

TEST(PixelSizingTest, PipelineRendersAtTheRequestedResolutionBeyondTheScreen) {
    // A 1 m square of strips 1.2 mm tall and 20 mm long: 83300 triangles about 1.2 mm wide.
    // At num_pixel = 4000 the frustum (2R = 1.414 m) gives 0.35 mm pixels, 3.4 across each
    // triangle, so CoP must see all of them. Rendered instead at screen size -- which the
    // viewport silently was, e.g. 1924 x 1175, until it was set explicitly -- the strips are
    // about one pixel tall and neighbouring centroids overwrite each other.
    const int nx = 50;
    const int ny = 833;
    MeshData panel{"strips", {}, {}};
    for (int j = 0; j <= ny; ++j) {
        for (int i = 0; i <= nx; ++i) {
            panel.positions.insert(panel.positions.end(),
                {-0.5f + static_cast<float>(i) / nx, -0.5f + static_cast<float>(j) / ny, 0.0f});
        }
    }
    for (int j = 0; j < ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            const std::uint32_t a = j * (nx + 1) + i, b = a + 1, c = a + (nx + 1), d = c + 1;
            panel.indices.insert(panel.indices.end(), {a, b, d, a, d, c}); // facing +z
        }
    }
    StaticMeshGeometry geometry(std::vector<MeshData>{panel});

    const unsigned int num_pixel = 4000;
    ShadingPipeline pipeline(geometry, ShadingAlgorithmType::CoP, num_pixel);
    const std::vector<float> visibility = pipeline.shade(glm::vec3(0.0f, 0.0f, 1.0f));

    GLint viewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, viewport);
    EXPECT_EQ(viewport[2], static_cast<GLint>(num_pixel));
    EXPECT_EQ(viewport[3], static_cast<GLint>(num_pixel));

    const auto seen = std::count_if(visibility.begin(), visibility.end(), [](float v) { return v > 0.5f; });
    EXPECT_EQ(static_cast<std::size_t>(seen), visibility.size());
}

TEST(PixelSizingTest, PipelineRejectsZeroPixels) {
    StaticMeshGeometry tetra(DataPath("../geometries/tetraeder.obj"));
    EXPECT_THROW(ShadingPipeline(tetra, ShadingAlgorithmType::Binary, 0), std::invalid_argument);
}
