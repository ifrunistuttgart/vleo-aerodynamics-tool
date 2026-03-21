#include "pch.h"
#include "src/ShadingPipeline.h"
#include "src/BinaryShader/Binary_Shader.h"
#include "Core/ISatellite_shading_data.h"
#include "res/Geometries/tetraeder_vector.h"
#include "src/ShadingAlgorithmFactory.h"
#include <span>
#include <vector>
#include <cstdint>
#include <eigen3/Eigen/Dense>


class FakeSatelliteData final : public ISatelliteShadingData {
public:
    std::span<const float> get_vertices() override {
        return std::span<const float>(vertices.data(), vertices.size());
    }

    std::span<const std::uint32_t> get_triangle_ids() override {
        return std::span<const std::uint32_t>(triangleIDs.data(), triangleIDs.size());
    }

    std::span<const float> get_normals() override {
        return std::span<const float>();
    }

    std::span<const float> get_areas() override {
        return std::span<const float>();
    }

    std::span<const float> get_centroids() override {
        return std::span<const float>();
    }

    std::span<const glm::mat4> get_model_matrices() override {
        std::vector<glm::mat4> model_matrices{ glm::mat4(1.0f) }; // Identity matrix for the single mesh
        return std::span<const glm::mat4>(model_matrices.data(), model_matrices.size());
	}

    std::span<const unsigned int> get_num_triangles_per_mesh() override {
        std::vector<unsigned int> num_triangles_per_mesh{ static_cast<unsigned int>(triangleIDs.size()) };
        return std::span<const unsigned int>(num_triangles_per_mesh.data(), num_triangles_per_mesh.size());
	}

    const unsigned int get_num_triangles() override {
        return static_cast<unsigned int>(triangleIDs.size());
    }

    float get_bounding_sphere_radius() override {
        return 0.5f;
    }
};

TEST(ShadingPipelineTests, TestShading) {
    FakeSatelliteData sat;
    ShadingPipeline pipeline(sat, ShadingAlgorithmType::Binary, 800);

    std::vector<float> isTriangleVisible(triangleIDs.size(), 0.0f);
    pipeline.shade(std::span<float>(isTriangleVisible), Eigen::Vector3f(1.0f, 0.0f, 0.0f));

    EXPECT_NEAR(isTriangleVisible[0], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[1], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[2], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[3], 1.0f, 1e-5);
}