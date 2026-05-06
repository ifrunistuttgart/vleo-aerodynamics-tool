#pragma once
#include "core.h"
#include <glm/glm.hpp>

class IAeroCalculator {
public:
    /**
     * @param v_rel__m_per_s
     * @param aero
     */
    virtual int calc_aero_torque_force(const glm::vec3& v_rel__m_per_s, float surface_temp__K, AeroConditions aero, glm::vec3& torque__Nm, glm::vec3& force__N) = 0;

    virtual ~IAeroCalculator() = default;
};