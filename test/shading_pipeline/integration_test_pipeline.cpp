#include <gtest/gtest.h>
#define FMT_UNICODE 0
#include <spdlog/spdlog.h>
#include <memory>
#include <span>
#include <vector>
#include <glm/glm.hpp>
#include <filesystem>

#include "shading_pipeline.h"
#include "shading_algorithm_factory.h"
#include "geometries/tetraeder_vector.h"
#include "static_mesh_geometry.h"


using namespace vat;

// Get path relative to this source file
std::string GetTestDataPath(const std::string& filename) {
    std::filesystem::path source_file(__FILE__);
    std::filesystem::path data_file = source_file.parent_path() / filename;
    return data_file.string();
}

// Test class for StaticMeshGeometry
class ShadingPipelineIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<StaticMeshGeometry> geometry;

    void SetUp() override {
        std::string obj_path = GetTestDataPath("geometries/tetraeder.obj");
		SPDLOG_INFO("[TEST] Loading OBJ from: {}", obj_path);
        geometry = std::make_unique<StaticMeshGeometry>(obj_path);
		SPDLOG_INFO("[TEST] Loaded {} triangles", geometry->get_num_triangles());
    }
};


TEST_F(ShadingPipelineIntegrationTest, TestShading) {
    ShadingPipeline pipeline(*geometry, ShadingAlgorithmType::Binary, 800);

    std::vector<float> isTriangleVisible(geometry->get_num_triangles(), 0.0f);
    pipeline.shade(std::span<float>(isTriangleVisible), glm::vec3(1.0f, 0.0f, 0.0f));

    EXPECT_NEAR(isTriangleVisible[0], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[1], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[2], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[3], 1.0f, 1e-5);
}