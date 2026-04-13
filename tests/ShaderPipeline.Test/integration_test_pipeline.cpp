#include "pch.h"
#include "src/ShadingPipeline.h"
#include "src/ShadingAlgorithmFactory.h"
#include <span>
#include <vector>
#include <glm/glm.hpp>
#include "tetraeder_vector.h"
#include "StaticMeshSatellite.h"
#include <filesystem>
#include <iostream>
#include <memory>


// Get path relative to this source file
std::string GetTestDataPath(const std::string& filename) {
    std::filesystem::path source_file(__FILE__);
    std::filesystem::path data_file = source_file.parent_path() / filename;
    return data_file.string();
}

// Test class for StaticMeshSatellite
class ShadingPipelineIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<StaticMeshSatellite> satellite;

    void SetUp() override {
        std::string obj_path = GetTestDataPath("tetraeder.obj");
        std::cout << "[TEST] Loading OBJ from: " << obj_path << std::endl;
        satellite = std::make_unique<StaticMeshSatellite>(obj_path);
        std::cout << "[TEST] Loaded " << satellite->get_num_triangles() << " triangles" << std::endl;
    }
};


TEST_F(ShadingPipelineIntegrationTest, TestShading) {
    ShadingPipeline pipeline(*satellite, ShadingAlgorithmType::Binary, 800);

    std::vector<float> isTriangleVisible(triangleIDs.size(), 0.0f);
    pipeline.shade(std::span<float>(isTriangleVisible), glm::vec3(1.0f, 0.0f, 0.0f));

    EXPECT_NEAR(isTriangleVisible[0], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[1], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[2], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[3], 1.0f, 1e-5);
}