#pragma once
#include <memory>
#include <string>
#include <span>
#include <glm/glm.hpp>
#include "static_mesh_satellite.h"

class RotatableMeshSatellite : public StaticMeshSatellite {
protected:
	std::vector<float> m_transformed_vertices;
	std::vector<float> m_transformed_normals;
	std::vector<float> m_transformed_centroids;
public:
    RotatableMeshSatellite(std::string file);
    ~RotatableMeshSatellite() = default;

    //ISatelliteShadingData interface
    // each vertex has three values (x, y, z)
    std::span<const float> get_vertices() override;

    // three values per triangle 
    std::span<const float> get_normals() override;

    // three values per triangle
    std::span<const float> get_centroids() override;

    float get_bounding_sphere_radius() override;

    //ISatelliteManipulator interface
    int turn_surface_around_axis(const int surface_id, float angle__rad, const std::array<float, 3>& origin, const std::array<float, 3>& axis) override;

private:
	std::vector<float> apply_transform(std::span<float> coordinates, int num_entries_per_triangle);
};