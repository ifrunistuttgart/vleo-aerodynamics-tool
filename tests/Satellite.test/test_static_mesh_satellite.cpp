#include "pch.h"
#include "tetraeder_vector.h"
#include "StaticMeshSatellite.h"
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <iostream>

// Get path relative to this source file
std::string GetTestDataPath(const std::string& filename) {
    std::filesystem::path source_file(__FILE__);
    std::filesystem::path data_file = source_file.parent_path() / filename;
    return data_file.string();
}

// Test class for StaticMeshSatellite
class StaticMeshSatelliteTest : public ::testing::Test {
protected:
    StaticMeshSatellite* satellite = nullptr;

    void SetUp() override {
        std::string obj_path = GetTestDataPath("tetraeder.obj");
        std::cout << "[TEST] Loading OBJ from: " << obj_path << std::endl;
        satellite = new StaticMeshSatellite(obj_path);
        std::cout << "[TEST] Loaded " << satellite->get_num_triangles() << " triangles" << std::endl;
    }

    void TearDown() override {
        delete satellite;
    }
};

// Test: Load the OBJ file successfully
TEST_F(StaticMeshSatelliteTest, LoadOBJFileSuccessfully) {
    ASSERT_NE(satellite, nullptr);
    EXPECT_GT(satellite->get_num_triangles(), 0);
}

// Test: Verify vertex count
TEST_F(StaticMeshSatelliteTest, VerifyVertexCount) {
    auto vertices = satellite->get_vertices();
    // Tetrahedron has 4 unique faces, with 3 vertices per face and 3 floats per vertex
    // we expect to have 36 values in the vertex array
    EXPECT_EQ(vertices.size(), 36);
}

// Test: Verify triangle count
TEST_F(StaticMeshSatelliteTest, VerifyTriangleCount) {
    EXPECT_EQ(satellite->get_num_triangles(), 4);
}

// Test: Verify triangle indices
TEST_F(StaticMeshSatelliteTest, VerifyTriangleIndices) {
    auto triangle_ids = satellite->get_triangle_ids();
    // 4 triangles * 3 indices per triangle = 12 indices
    EXPECT_EQ(triangle_ids.size(), 12);
}

// Test: Verify normals are computed
TEST_F(StaticMeshSatelliteTest, VerifyNormals) {
    auto normals = satellite->get_normals();
    // 4 triangles * 3 values per normal = 12 values
    EXPECT_EQ(normals.size(), 12);
    float expected_normals[12] = {    0.0f,    0.0f,   -1.0f,
                                  -0.5774f, 0.5774f, 0.5774f,
                                  -0.5774f,-0.5774f, 0.5774f,
								      1.0f,    0.0f,    0.0f};
    // Each normal should be normalized (length close to 1)
    for (size_t i = 0; i < normals.size(); i += 3) {
		EXPECT_NEAR(normals[i], expected_normals[i], 1e-4f)
			<< "Normal x at index " << i << " is " << normals[i];
    }
}

// Test: Verify centroids are computed
TEST_F(StaticMeshSatelliteTest, VerifyCentroids) {
    auto centroids = satellite->get_centroids();
    // 4 triangles * 3 values per centroid = 12 values
    EXPECT_EQ(centroids.size(), 12);
    float cntroids[12] = { -0.1667f,    0.0f,    0.0f,
                           -0.1667f, 0.1667f, 0.1667f,
                           -0.1667f,-0.1667f, 0.1667f,
                               0.0f,    0.0f, 0.1667f };
    // Verify centroids are within reasonable bounds
    for (size_t i = 0; i < centroids.size(); i += 3) {
        EXPECT_NEAR(centroids[i], cntroids[i], 1e-4f) 
			<< "Centroid x at index " << i << " is " << centroids[i];
    }
}

// Test: Verify areas are computed
TEST_F(StaticMeshSatelliteTest, VerifyAreas) {
    auto areas = satellite->get_areas();
    // 4 triangles = 4 area values
    EXPECT_EQ(areas.size(), 4);
    
	EXPECT_NEAR(areas[0], 0.25f, 1e-4f); // Base triangle area
	EXPECT_NEAR(areas[1], 0.2165f, 1e-4f); // Side triangle area
	EXPECT_NEAR(areas[2], 0.2165f, 1e-4f); // Side triangle area
	EXPECT_NEAR(areas[3], 0.25f, 1e-4f); // Side triangle area
    // All areas should be positive
    for (float area : areas) {
        EXPECT_GT(area, 0.0f) << "Triangle area should be positive";
    }
}

// Test: Verify triangles per mesh
TEST_F(StaticMeshSatelliteTest, VerifyTrianglesPerMesh) {
    auto tri_per_mesh = satellite->get_num_triangles_per_mesh();
    EXPECT_EQ(tri_per_mesh.size(), 1); // Single mesh
    EXPECT_EQ(tri_per_mesh[0], 4); // 4 triangles
}

// Test: Verify bounding sphere radius
TEST_F(StaticMeshSatelliteTest, VerifyBoundingSphereRadius) {
    float radius = satellite->get_bounding_sphere_radius();
    
    // The maximum distance from origin should be around 0.707 (sqrt(0.5^2 + 0.5^2))
    // but could be different depending on vertex ordering
    EXPECT_GT(radius, 0.0f);
    
    // Verify it's within reasonable bounds for the tetrahedron
    EXPECT_LT(radius, 1.0f);
}

// Test: Verify vertices match ground truth (approximately)
TEST_F(StaticMeshSatelliteTest, VerifyVerticesMatchGroundTruth) {
    auto vertices = satellite->get_vertices();
    
    // Collect unique vertices from loaded data
    std::vector<float> loaded_vertices(vertices.begin(), vertices.end());
    
    // The ground truth vertices
    const std::vector<float>& expected_vertices = ::vertices;
    
    // Both should have same size
    EXPECT_EQ(loaded_vertices.size(), expected_vertices.size());
    
    // Check that all loaded vertices match expected vertices
    for (size_t i = 0; i < std::min(loaded_vertices.size(), expected_vertices.size()); ++i) {
        EXPECT_NEAR(loaded_vertices[i], expected_vertices[i], 1e-5)
            << "Vertex value at index " << i << " doesn't match. "
            << "Expected: " << expected_vertices[i] << ", Got: " << loaded_vertices[i];
    }
}
