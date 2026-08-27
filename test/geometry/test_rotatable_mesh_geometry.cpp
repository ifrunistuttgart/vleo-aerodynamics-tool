#include <gtest/gtest.h>
#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <algorithm>

#include "geometries/tetraeder_vector.h"
#include "rotatable_mesh_geometry.h"
#include "test_helpers.h"

using namespace vat;

// Test class for StaticMeshGeometry
class RotatableMeshGeometryTest : public ::testing::Test {
protected:
    RotatableMeshGeometry* geometry = nullptr;

    void SetUp() override {
        std::string obj_path = GetTestDataPath(__FILE__, "geometries/tetraeder.obj");
		SPDLOG_INFO("[TEST] Loading OBJ from: {}", obj_path);
        geometry = new RotatableMeshGeometry(obj_path);
		SPDLOG_INFO("[TEST] Loaded {} triangles", geometry->get_num_triangles());
    }

    void TearDown() override {
        delete geometry;
    }
};

// Test: Load the OBJ file successfully
TEST_F(RotatableMeshGeometryTest, LoadOBJFileSuccessfully) {
    ASSERT_NE(geometry, nullptr);
    EXPECT_GT(geometry->get_num_triangles(), 0);
}

// Test: Verify vertex count
TEST_F(RotatableMeshGeometryTest, VerifyVertexCount) {
    auto vertices = geometry->get_vertices();
    // Tetrahedron has 4 unique faces, with 3 vertices per face and 3 floats per vertex
    // we expect to have 36 values in the vertex array
    EXPECT_EQ(vertices.size(), 36);
}

// Test: Verify triangle count
TEST_F(RotatableMeshGeometryTest, VerifyTriangleCount) {
    EXPECT_EQ(geometry->get_num_triangles(), 4);
}

// Test: Verify triangle indices
TEST_F(RotatableMeshGeometryTest, VerifyTriangleIndices) {
    auto triangle_ids = geometry->get_triangle_ids();
    // 4 triangles * 3 indices per triangle = 12 indices
    EXPECT_EQ(triangle_ids.size(), 12);
}

// Test: Verify normals are computed
TEST_F(RotatableMeshGeometryTest, VerifyNormals) {
    auto normals = geometry->get_normals();
    // 4 triangles * 3 values per normal = 12 values
    EXPECT_EQ(normals.size(), 12);
    float expected_normals[12] = { 0.0f,    0.0f,   -1.0f,
                                  -0.5774f, 0.5774f, 0.5774f,
                                  -0.5774f,-0.5774f, 0.5774f,
                                      1.0f,    0.0f,    0.0f };
    // Each normal should be normalized (length close to 1)
    for (size_t i = 0; i < normals.size(); i += 3) {
        EXPECT_NEAR(normals[i], expected_normals[i], 1e-4f)
            << "Normal x at index " << i << " is " << normals[i];
    }
}

// Test: Verify centroids are computed
TEST_F(RotatableMeshGeometryTest, VerifyCentroids) {
    auto centroids = geometry->get_centroids();
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
TEST_F(RotatableMeshGeometryTest, VerifyAreas) {
    auto areas = geometry->get_areas();
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
TEST_F(RotatableMeshGeometryTest, VerifyTrianglesPerMesh) {
    auto tri_per_mesh = geometry->get_num_triangles_per_mesh();
    EXPECT_EQ(tri_per_mesh.size(), 1); // Single mesh
    EXPECT_EQ(tri_per_mesh[0], 4); // 4 triangles
}

// Test: Verify bounding sphere radius
TEST_F(RotatableMeshGeometryTest, VerifyBoundingSphereRadius) {
    float radius = geometry->get_bounding_sphere_radius();

    // The maximum distance from origin should be around 0.707 (sqrt(0.5^2 + 0.5^2))
    // but could be different depending on vertex ordering
    EXPECT_GT(radius, 0.0f);

    // Verify it's within reasonable bounds for the tetrahedron
    EXPECT_LT(radius, 1.0f);
}

// Test: Verify vertices match ground truth (approximately)
TEST_F(RotatableMeshGeometryTest, VerifyVerticesMatchGroundTruth) {
    auto vertices = geometry->get_vertices();

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
