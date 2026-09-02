#pragma once
#include <memory>
#include <string>
#include <span>
#include <glm/glm.hpp>
#include "static_mesh_satellite.h"

/*
 * A satellite class that allows for rotation of its surfaces. It inherits from StaticMeshSatellite and implements the ISatelliteManipulator interface.
 * This class maintains transformed vertices, normals, and centroids to reflect the changes in the satellite's configuration after rotations.
 */
class RotatableMeshSatellite : public StaticMeshSatellite {
protected:
	std::vector<float> m_transformed_vertices;
	std::vector<float> m_transformed_normals;
	std::vector<float> m_transformed_centroids;
    bool m_transformed_data_current = false;

    void refresh_transformed_data();
public:
    RotatableMeshSatellite(std::string file);
    ~RotatableMeshSatellite() = default;

    std::span<const float> get_vertices() override;

    std::span<const float> get_raw_vertices() override;

    std::span<const float> get_normals() override;

    std::span<const float> get_centroids() override;

    float get_bounding_sphere_radius() override;

    int turn_surface_around_axis(const int surface_id, float angle__rad, const std::array<float, 3>& origin, const std::array<float, 3>& axis) override;

private:
	std::vector<float> apply_transform(std::span<float> coordinates, int num_entries_per_triangle);
	std::vector<float> apply_normal_transform(std::span<float> normals, int num_entries_per_triangle);
};