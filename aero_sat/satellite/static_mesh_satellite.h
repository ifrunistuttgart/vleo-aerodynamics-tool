#pragma once
#include <memory>
#include <string>
#include <vector>
#include <span>
#include <glm/glm.hpp>
#include "Isatellite_shading_data.h"
#include "Isatellite_manipulator.h"


/**
* A satellite class with a static mesh structure. It implements both the ISatelliteShadingData and ISatelliteManipulator interfaces. 
* Altough it implements the ISatelliteManipulator interface, the turn_surface and turn_surface_around_axis functions do not actually change the satellite's configuration, since the mesh is static.
* This class is useful for testing and benchmarking purposes, as it provides a simple implementation of the satellite interfaces without the complexity of handling dynamic transformations.
*/
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

    std::span<const float> get_vertices() override;
    std::span<const float> get_model_space_vertices() override;
    std::span<const std::uint32_t> get_triangle_ids() override;
    std::span<const float> get_normals() override;
    std::span<const float> get_areas() override;
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