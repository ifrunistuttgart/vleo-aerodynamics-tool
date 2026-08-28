#include <gtest/gtest.h>
#include <span>
#include <vector>
#include <cstdint>
#include <string>

#include "shading_pipeline.h"
#include "binary_shader/binary_shader.h"
#include "Igeometry_shading_data.h"
#include "geometries/tetraeder_vector.h"
#include "shading_algorithm_factory.h"

using namespace vat;


class FakeGeometryData final : public IGeometryShadingData {
private:
    std::vector<glm::mat4> m_model_matrices{ glm::mat4(1.0f) };
    std::vector<unsigned int> m_num_triangles_per_mesh{ static_cast<unsigned int>(triangleIDs.size() / 3) };
    std::vector<std::string> m_mesh_names{ "Mesh 0" };

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
        return std::span<const glm::mat4>(m_model_matrices.data(), m_model_matrices.size());
	}

    std::span<const std::string> get_mesh_names() override {
        return std::span<const std::string>(m_mesh_names.data(), m_mesh_names.size());
	}

    std::span<const unsigned int> get_num_triangles_per_mesh() override {
        return std::span<const unsigned int>(m_num_triangles_per_mesh.data(), m_num_triangles_per_mesh.size());
	}

    const unsigned int get_num_triangles() override {
        return static_cast<unsigned int>(triangleIDs.size() / 3);
    }

    float get_bounding_sphere_radius() override {
        return 0.5f;
    }
};

TEST(ShadingPipelineTests, TestShading) {
    FakeGeometryData sat;
    ShadingPipeline pipeline(sat, ShadingAlgorithmType::Binary, 800);

    std::vector<float> isTriangleVisible(triangleIDs.size(), 0.0f);
    pipeline.shade(std::span<float>(isTriangleVisible), glm::vec3(1.0f, 0.0f, 0.0f));

    EXPECT_NEAR(isTriangleVisible[0], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[1], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[2], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[3], 1.0f, 1e-5);
}