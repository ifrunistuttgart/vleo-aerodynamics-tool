#pragma once
#include <memory>

/**
 * Interface for manipulating satellite surfaces
 */
class ISatelliteManipulator {
public:
	/**
	 * Turn a specific surface of the satellite
	 * 
	 * @param surface_id - ID of the surface to turn
	 * @param angle__rad - Angle to turn the surface by, in radians
	 * @return - Status code indicating success or failure
	 */
	virtual int turn_surface(int surface_id, float angle__rad) = 0;

	/**
	 * Turn a specific surface of the satellite around a specified axis
	 * 
	 * @param surface_id - ID of the surface to turn
	 * @param angle__rad - Angle to turn the surface by, in radians
	 * @param origin - Origin point for the rotation (3D vector)
	 * @param axis - Axis to rotate around (3D vector)
	 * @return - Status code indicating success or failure
	 */
	virtual int turn_surface_around_axis(const int surface_id, float angle__rad, const std::array<float,3>& origin, const std::array<float,3>& axis) = 0;

	/**
	 * Turn all surfaces of the satellite
	 * @return - Status code indicating success or failure
	 */
	virtual int turn_surfaces() = 0;

	virtual ~ISatelliteManipulator() = default;
};