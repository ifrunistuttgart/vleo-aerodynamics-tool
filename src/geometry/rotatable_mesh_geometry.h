#pragma once
#include <memory>
#include <string>
#include <span>
#include <glm/glm.hpp>
#include "static_mesh_geometry.h"

namespace vat::geometry {

/*
 * A geometry class that allows rotation of its meshes. It inherits from StaticMeshGeometry and implements the IGeometryManipulator interface.
 * This class maintains transformed vertices, normals, and centroids to reflect the changes in the geometry's configuration after rotations.
 */
class RotatableMeshGeometry : public StaticMeshGeometry {
protected:
	std::vector<float> m_transformed_vertices;
	std::vector<float> m_transformed_normals;
	std::vector<float> m_transformed_centroids;
	bool m_transforms_outdated = true;
public:
    RotatableMeshGeometry(std::string file);
    ~RotatableMeshGeometry() = default;

    std::span<const float> get_vertices() override;

    std::span<const float> get_raw_vertices() override;

    std::span<const float> get_normals() override;

    std::span<const float> get_centroids() override;

    float get_bounding_sphere_radius() override;

    int turn_mesh_around_axis(const int mesh_id, float angle__rad, const std::array<float, 3>& origin, const std::array<float, 3>& axis) override;

private:
	/**
	 * Recomputes the transformed geometry and the bounding sphere, but only if a mesh
	 * has been turned since the last call.
	 *
	 * All three arrays and the radius are refreshed together because one force/torque
	 * evaluation needs all of them and they all derive from the same model matrices.
	 * Sweeping the flow direction at a fixed pose - the toolbox's main use case -
	 * therefore pays for the transform once instead of on every getter call.
	 */
	void refresh_transforms();

	/**
	 * Applies each mesh's model matrix to position data (vertices, centroids).
	 *
	 * @param coordinates Untransformed coordinates, mesh blocks in load order.
	 * @param num_entries_per_triangle Floats one triangle occupies (9 for vertices, 3 for centroids).
	 * @param target Pre-sized destination, written in place so a span handed out by an
	 *               earlier getter call stays valid across a refresh.
	 */
	void apply_transform(std::span<const float> coordinates, int num_entries_per_triangle, std::vector<float>& target) const;

	/**
	 * Applies each mesh's normal matrix to direction data (normals).
	 *
	 * Uses the inverse transpose of the linear block, which stays correct if a
	 * non-uniform scale is ever introduced, then renormalizes: Sentman divides only by
	 * |v_rel| and so requires unit normals.
	 *
	 * @param target Pre-sized destination, written in place. See apply_transform.
	 */
	void apply_normal_transform(std::span<const float> normals, int num_entries_per_triangle, std::vector<float>& target) const;
};

} // namespace vat::geometry
