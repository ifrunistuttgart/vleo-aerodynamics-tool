#pragma once
#include "IGSI_Model.h"
#include <eigen3/Eigen/Dense>


class Schuette: public IGSIModel {
public:
	/**
	* @param cl_cd_lut
	*/
    Schuette(int cl_cd_lut);
	~Schuette() override;

    int calc_aero_force_and_torque(float area__m2, const Eigen::Vector3f& normal, const Eigen::Vector3f& centroid__m, const Eigen::Vector3f& v_rel__m_per_s, float surf_temp__K, AeroConditions aero, Eigen::Vector3f& aero_force__N, Eigen::Vector3f& aero_torque__Nm) override;

private:
	int cl_cd_lut;
};