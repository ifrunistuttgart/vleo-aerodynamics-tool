#pragma once


class ISatelliteManipulator {
public:
	virtual int turn_surface(int surface_id, float angle__rad) = 0;
	virtual int turn_surfaces() = 0;
	virtual ~ISatelliteManipulator() = default;
};