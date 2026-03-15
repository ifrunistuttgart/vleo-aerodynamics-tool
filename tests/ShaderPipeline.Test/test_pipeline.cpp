#include "pch.h"
#include "src/ShadingPipeline.h"
#include "src/BinaryShader/Binary_Shader.h"
#include "Core/ISatellite_shading_data.h"
#include "res/Geometries/tetraeder.h"
#include "src/ShadingAlgorithmFactory.h"
#include <eigen3/Eigen/Dense>


class FakeSatelliteData final : public ISatelliteShadingData {
public:

    float* get_vertices() override { return vertices; }
    size_t get_num_vertices() override { return sizeof(vertices)/sizeof(float); }
    unsigned int* get_triangle_ids() override { return triangleIDs; }
    size_t get_num_triangle_ids() override { return sizeof(triangleIDs)/sizeof(unsigned int); }
    float get_bounding_sphere_radius() override { return 0.5f; }
};

TEST(ShadingPipelineTests, TestShading) {
    FakeSatelliteData sat;
    ShadingPipeline pipeline(sat, ShadingAlgorithmType::Binary, 800);
    float isTriangleVisible[sizeof(triangleIDs) / sizeof(unsigned int)] = {0};
	pipeline.shade(isTriangleVisible, Eigen::Vector3f(1.0f, 0.0f, 0.0f));

    EXPECT_NEAR(isTriangleVisible[0], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[1], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[2], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[3], 1.0f, 1e-5);
}