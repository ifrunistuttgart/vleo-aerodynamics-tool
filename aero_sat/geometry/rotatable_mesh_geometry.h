#pragma once
#include <memory>
#include <string>
#include <span>
#include <glm/glm.hpp>
#include "static_mesh_geometry.h"

namespace vat {

/*
 * A geometry whose individual meshes can be rotated. It inherits from StaticMeshGeometry and implements the IGeometryManipulator interface.
 * This class maintains transformed vertices, normals, and centroids to reflect the changes in the geometry's configuration after rotations.
 */
class RotatableMeshGeometry : public StaticMeshGeometry {
protected:
	std::vector<float> m_transformed_vertices;
	std::vector<float> m_transformed_normals;
	std::vector<float> m_transformed_centroids;
public:
    RotatableMeshGeometry(std::string file);
    ~RotatableMeshGeometry() = default;

    std::span<const float> get_vertices() override;

    std::span<const float> get_normals() override;

    std::span<const float> get_centroids() override;

    float get_bounding_sphere_radius() override;

    int turn_mesh_around_axis(const int mesh_id, float angle__rad, const std::array<float, 3>& origin, const std::array<float, 3>& axis) override;

private:
	std::vector<float> apply_transform(std::span<float> coordinates, int num_entries_per_triangle);
};

} // namespace vat
