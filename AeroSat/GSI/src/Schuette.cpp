#include "Schuette.h"
#include <eigen3/Eigen/Dense>

/**
 * @param cl_cd_lut
 */
Schuette::Schuette(int cl_cd_lut) {
	Schuette::cl_cd_lut = cl_cd_lut;
}

Schuette::~Schuette() {
}

int Schuette::calc_aero_force_and_torque(float area__m2, const Eigen::Vector3f& normal, const Eigen::Vector3f& centroid__m, const Eigen::Vector3f& v_rel__m_per_s, float surf_temp__K, AeroConditions aero, Eigen::Vector3f& aero_force__N, Eigen::Vector3f& aero_torque__Nm) {
	return 0;
}


