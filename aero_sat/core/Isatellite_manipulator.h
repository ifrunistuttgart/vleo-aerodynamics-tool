#pragma once
#include <memory>

/**
 * Interface for manipulating satellite surfaces and components.
 *
 * This interface allows for programmatic rotation of individual surfaces or meshes
 * of a satellite, which is essential for simulating articulated parts like solar arrays.
 */
class ISatelliteManipulator {
public:
	/**
	 * Rotates a specific surface or mesh of the satellite.
	 *
	 * @param surface_id The ID of the surface or mesh to rotate.
	 * @param angle__rad The angle of rotation in radians.
	 * @return 0 on success, or a non-zero error code on failure (e.g., if the ID is invalid).
	 */
	virtual int turn_surface(int surface_id, float angle__rad) = 0;

	/**
	 * Rotates a specific surface or mesh of the satellite around a custom axis and origin.
	 *
	 * @param surface_id The ID of the surface or mesh to rotate.
	 * @param angle__rad The angle of rotation in radians.
	 * @param origin The 3D coordinates of the origin point for the rotation.
	 * @param axis The 3D vector defining the axis of rotation.
	 * @return 0 on success, or a non-zero error code on failure.
	 */
	virtual int turn_surface_around_axis(const int surface_id, float angle__rad, const std::array<float,3>& origin, const std::array<float,3>& axis) = 0;

	/**
	 * Rotates all surfaces of the satellite according to a pre-defined logic.
	 *
	 * @return 0 on success, or a non-zero error code on failure.
	 */
	virtual int turn_surfaces() = 0;

	virtual ~ISatelliteManipulator() = default;
};