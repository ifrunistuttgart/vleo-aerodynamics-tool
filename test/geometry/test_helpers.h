#pragma once
#include <gtest/gtest.h>

#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "static_mesh_geometry.h"

// Get path relative to this source file
inline std::string get_test_data_path(const char* source_file, const std::string& filename) {
    std::filesystem::path source_path(source_file);
    std::filesystem::path data_file = source_path.parent_path() / filename;
    return data_file.string();
}

template <typename T> std::vector<T> to_vector(std::span<const T> s) { return std::vector<T>(s.begin(), s.end()); }

// Every derived array compared exactly, not within a tolerance.
inline void expect_same_geometry(vat::geometry::StaticMeshGeometry& a, vat::geometry::StaticMeshGeometry& b) {
    EXPECT_EQ(a.get_num_triangles(), b.get_num_triangles());
    EXPECT_EQ(to_vector(a.get_transformed_vertices()), to_vector(b.get_transformed_vertices()));
    EXPECT_EQ(to_vector(a.get_untransformed_vertices()), to_vector(b.get_untransformed_vertices()));
    EXPECT_EQ(to_vector(a.get_triangle_ids()), to_vector(b.get_triangle_ids()));
    EXPECT_EQ(to_vector(a.get_transformed_normals()), to_vector(b.get_transformed_normals()));
    EXPECT_EQ(to_vector(a.get_untransformed_normals()), to_vector(b.get_untransformed_normals()));
    EXPECT_EQ(to_vector(a.get_transformed_centroids()), to_vector(b.get_transformed_centroids()));
    EXPECT_EQ(to_vector(a.get_untransformed_centroids()), to_vector(b.get_untransformed_centroids()));
    EXPECT_EQ(to_vector(a.get_areas()), to_vector(b.get_areas()));
    EXPECT_EQ(to_vector(a.get_num_triangles_per_mesh()), to_vector(b.get_num_triangles_per_mesh()));
    EXPECT_EQ(to_vector(a.get_mesh_names()), to_vector(b.get_mesh_names()));
    EXPECT_EQ(a.get_bounding_sphere_radius(), b.get_bounding_sphere_radius());
}
