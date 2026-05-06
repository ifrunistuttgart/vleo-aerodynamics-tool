#pragma once
#include "Igsi_model.h"
#include <glm/glm.hpp>


class Schuette: public IGSIModel {
public:
	/**
	* @param cl_cd_lut
	*/
    Schuette(int cl_cd_lut);
	~Schuette() override;

    int calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m, const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions aero, glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) override;

private:
	int cl_cd_lut;
};