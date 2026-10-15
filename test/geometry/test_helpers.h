#pragma once
#include <gtest/gtest.h>

#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "static_mesh_geometry.h"

// Get path relative to this source file
inline std::string GetTestDataPath(const char* sourceFile, const std::string& filename) {
    std::filesystem::path source_file(sourceFile);
    std::filesystem::path data_file = source_file.parent_path() / filename;
    return data_file.string();
}

template <typename T>
std::vector<T> ToVector(std::span<const T> s) {
    return std::vector<T>(s.begin(), s.end());
}

// Every derived array compared exactly, not within a tolerance.
inline void ExpectSameGeometry(vat::geometry::StaticMeshGeometry& a, vat::geometry::StaticMeshGeometry& b) {
    EXPECT_EQ(a.get_num_triangles(), b.get_num_triangles());
    EXPECT_EQ(ToVector(a.get_transformed_vertices()), ToVector(b.get_transformed_vertices()));
    EXPECT_EQ(ToVector(a.get_untransformed_vertices()), ToVector(b.get_untransformed_vertices()));
    EXPECT_EQ(ToVector(a.get_triangle_ids()), ToVector(b.get_triangle_ids()));
    EXPECT_EQ(ToVector(a.get_transformed_normals()), ToVector(b.get_transformed_normals()));
    EXPECT_EQ(ToVector(a.get_untransformed_normals()), ToVector(b.get_untransformed_normals()));
    EXPECT_EQ(ToVector(a.get_transformed_centroids()), ToVector(b.get_transformed_centroids()));
    EXPECT_EQ(ToVector(a.get_untransformed_centroids()), ToVector(b.get_untransformed_centroids()));
    EXPECT_EQ(ToVector(a.get_areas()), ToVector(b.get_areas()));
    EXPECT_EQ(ToVector(a.get_num_triangles_per_mesh()), ToVector(b.get_num_triangles_per_mesh()));
    EXPECT_EQ(ToVector(a.get_mesh_names()), ToVector(b.get_mesh_names()));
    EXPECT_EQ(a.get_bounding_sphere_radius(), b.get_bounding_sphere_radius());
}
