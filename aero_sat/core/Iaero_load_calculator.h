#pragma once
#include "core.h"
#include <glm/glm.hpp>

class IAeroLoadCalculator {
public:
    /**
     * Calculate the aerodynamic loads (torque and force) acting on an object under given VLEO conditions
     * 
	 * Calculates the aerodynamic torque and force on the satellite based on the relative velocity of the incoming flow, surface temperature, and atmospheric conditions.
     * The method should be implemented by derived classes.
     * 
	 * @param v_rel__m_per_s - relative velocity vector of the incoming flow in the satellite's body frame, in meters per second
     * @param surface_temp__K - surface temperature in Kelvin
	 * @param aero - struct containing the atmospheric conditions (density, temperature, particle mass, energy accommodation coefficient)
	 * @param torque__Nm - output parameter for the calculated aerodynamic torque in Newton-meters
	 * @param force__N - output parameter for the calculated aerodynamic force in Newtons
     */
    virtual int calc_aero_torque_force(const glm::vec3& v_rel__m_per_s, float surface_temp__K, AeroConditions aero, glm::vec3& torque__Nm, glm::vec3& force__N) = 0;

    virtual ~IAeroLoadCalculator() = default;
};