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
public:
    RotatableMeshSatellite(std::string file);
    ~RotatableMeshSatellite() = default;

    std::span<const float> get_vertices() override;

    std::span<const float> get_normals() override;

    std::span<const float> get_centroids() override;

    float get_bounding_sphere_radius() override;

    int turn_surface_around_axis(const int surface_id, float angle__rad, const std::array<float, 3>& origin, const std::array<float, 3>& axis) override;

private:
	/**
	 * Applies each mesh's model matrix to position data (vertices, centroids).
	 *
	 * @param source Untransformed coordinates, mesh blocks in load order.
	 * @param floats_per_triangle Number of floats one triangle occupies (9 for vertices, 3 for centroids).
	 * @param target Pre-sized destination, written in place so previously returned spans stay valid.
	 */
	void transform_positions(std::span<const float> source, int floats_per_triangle, std::vector<float>& target) const;

	/**
	 * Applies each mesh's model matrix to direction data (normals).
	 *
	 * Directions transform by the linear block only - unlike a position, a direction
	 * must not pick up the matrix's translation column.
	 */
	void transform_directions(std::span<const float> source, std::vector<float>& target) const;
};