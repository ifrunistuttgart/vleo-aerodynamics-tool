#include "Schuette.h"
#include <glm/glm.hpp>

/**
 * @param cl_cd_lut
 */
Schuette::Schuette(int cl_cd_lut) {
	Schuette::cl_cd_lut = cl_cd_lut;
}

Schuette::~Schuette() {
}

int Schuette::calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m, const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions aero, glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) {
	return 0;
}


