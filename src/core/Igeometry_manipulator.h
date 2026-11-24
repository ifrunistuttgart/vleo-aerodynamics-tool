#pragma once
#include <memory>

namespace vat {

/**
 * @brief Interface for manipulating the meshes of a geometry.
 *
 * This interface allows for programmatic rotation of individual meshes
 * of a geometry, which is essential for simulating articulated parts like solar arrays.
 * Turning a mesh only changes its model matrix (see IGeometryShadingData::get_model_matrices());
 * the untransformed vertex data is never modified.
 */
class IGeometryManipulator {
public:
    /**
     * @brief Rotates a specific mesh of the geometry about a predefined axis.
     *
     * Not implemented by any geometry in this toolbox: StaticMeshGeometry and
     * RotatableMeshGeometry always return -1 and leave the geometry unchanged. Use
     * turn_mesh_around_axis() instead.
     *
     * @param mesh_id The ID of the mesh to rotate.
     * @param angle__rad The angle of rotation in radians.
     * @return 0 on success, or a non-zero error code on failure.
     */
    virtual int turn_mesh(int mesh_id, float angle__rad) = 0;

    /**
     * @brief Sets the pose of one mesh to a rotation about a custom axis and origin.
     *
     * The rotation replaces the mesh's model matrix and is applied to the untransformed mesh:
     * calls do not accumulate, so calling twice with the same angle gives the same pose.
     *
     * @param mesh_id 0-based mesh index, the index into IGeometryShadingData::get_mesh_names().
     * @param angle__rad The angle of rotation in radians (right-handed about axis).
     * @param origin Point on the rotation axis, in the body frame [m].
     * @param axis Direction of the rotation axis in the body frame. Need not be normalised, but
     *             must not be zero.
     * @return 0 on success, or a non-zero error code on failure. RotatableMeshGeometry returns -1
     *         for an out-of-range mesh_id; StaticMeshGeometry always returns -1.
     */
    virtual int turn_mesh_around_axis(const int mesh_id, float angle__rad, const std::array<float, 3>& origin,
                                      const std::array<float, 3>& axis) = 0;

    /**
     * @brief Rotates all meshes of the geometry according to a predefined logic.
     *
     * Not implemented by any geometry in this toolbox: StaticMeshGeometry and
     * RotatableMeshGeometry always return -1 and leave the geometry unchanged.
     *
     * @return 0 on success, or a non-zero error code on failure.
     */
    virtual int turn_meshes() = 0;

    virtual ~IGeometryManipulator() = default;
};

} // namespace vat
