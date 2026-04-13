#pragma once
#include <memory>
#include <string>
#include <vector>
#include <span>
#include <glm/glm.hpp>
#include "Core/ISatellite_shading_data.h"
#include "Core/ISatellite_Manipulator.h"

class StaticMeshSatellite: public ISatelliteShadingData, public ISatelliteManipulator {
protected:
    std::vector<float> m_vertices;
    std::vector<std::uint32_t> m_triangle_ids;
    std::vector<float> m_normals;
    std::vector<float> m_areas;
    std::vector<float> m_centroids;
	std::vector<glm::mat4> m_model_matrices;
    std::vector<unsigned int> m_num_triangles_per_mesh;
    unsigned int m_total_triangles;
    float m_bounding_sphere_radius;

public:
	StaticMeshSatellite(std::string file);
    ~StaticMeshSatellite() = default;

	//ISatelliteShadingData interface
    // each vertex has three values (x, y, z)
    std::span<const float> get_vertices() override;

    // one per vertex
    std::span<const std::uint32_t> get_triangle_ids() override;

    // three values per triangle 
    std::span<const float> get_normals() override;

    // one value per triangle
    std::span<const float> get_areas() override;

    // three values per triangle
    std::span<const float> get_centroids() override;

    std::span<const glm::mat4> get_model_matrices() override;
    std::span<const unsigned int> get_num_triangles_per_mesh() override;
    const unsigned int get_num_triangles() override;
    float get_bounding_sphere_radius() override;

	//ISatelliteManipulator interface
    int turn_surface(int surface_id, float angle__rad) override;
    int turn_surface_around_axis(const int surface_id, float angle__rad, const std::array<float, 3>& origin, const std::array<float, 3>& axis) override;
    int turn_surfaces() override;
};