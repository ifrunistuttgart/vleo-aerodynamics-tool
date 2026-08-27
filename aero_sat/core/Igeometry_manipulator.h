#pragma once
#include <memory>

namespace vat {

/**
 * Interface for manipulating the meshes of a geometry.
 *
 * A geometry is made up of one or more meshes, each of which is a set of triangles.
 * This interface allows for programmatic rotation of individual meshes, which is
 * essential for simulating articulated parts like solar arrays.
 */
class IGeometryManipulator {
public:
	/**
	 * Rotates a specific mesh of the geometry.
	 *
	 * @param mesh_id The ID of the mesh to rotate.
	 * @param angle__rad The angle of rotation in radians.
	 * @return 0 on success, or a non-zero error code on failure (e.g., if the ID is invalid).
	 */
	virtual int turn_mesh(int mesh_id, float angle__rad) = 0;

	/**
	 * Rotates a specific mesh of the geometry around a custom axis and origin.
	 *
	 * @param mesh_id The ID of the mesh to rotate.
	 * @param angle__rad The angle of rotation in radians.
	 * @param origin The 3D coordinates of the origin point for the rotation.
	 * @param axis The 3D vector defining the axis of rotation.
	 * @return 0 on success, or a non-zero error code on failure.
	 */
	virtual int turn_mesh_around_axis(const int mesh_id, float angle__rad, const std::array<float,3>& origin, const std::array<float,3>& axis) = 0;

	/**
	 * Rotates all meshes of the geometry according to a pre-defined logic.
	 *
	 * @return 0 on success, or a non-zero error code on failure.
	 */
	virtual int turn_meshes() = 0;

	virtual ~IGeometryManipulator() = default;
};

} // namespace vat
