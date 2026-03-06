#pragma once

#include <string>
#include "Core/ISatellite.h"
#include "Core/ISatellite_Manipulator.h"
#include "IKinematics.h"
#include "IGeometry.h"


class Satellite: public ISatellite, public ISatelliteManipulator {
private:
    IKinematics kinematics;
    IGeometry geometry;

public:
    Satellite(std::string file);

	// ISatelliteManipulator interface
    int turn_surface(int surface_id, float angle__rad) override;

    int turn_surfaces() override;

	// ISatellite interface
    int get_envelope() override;

    int get_vertices() override;

    int get_transformation_matrices() override;

	int get_areas() override;

	int get_normals() override;

	int get_centroids() override;

    int get_com() override;
};