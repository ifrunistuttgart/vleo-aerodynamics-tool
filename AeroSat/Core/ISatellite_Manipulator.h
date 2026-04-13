#pragma once
#include <memory>

class ISatelliteManipulator {
public:
	virtual int turn_surface(int surface_id, float angle__rad) = 0;
	virtual int turn_surface_around_axis(const int surface_id, float angle__rad, const std::array<float,3>& origin, const std::array<float,3>& axis) = 0;
	virtual int turn_surfaces() = 0;
	virtual ~ISatelliteManipulator() = default;
};