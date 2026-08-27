#pragma once
#include "core.h"
#include <glm/glm.hpp>

namespace vat {

/**
 * Interface for calculating aerodynamic loads on a satellite.
 *
 * This interface provides a common method for calculating aerodynamic torque and force
 * based on the satellite's geometry, its motion relative to the atmosphere, and
 * environmental conditions.
 */
class IAeroLoadCalculator {
public:
    /**
     * Calculates the aerodynamic torque and force acting on the satellite.
     *
     * This method evaluates the impact of gas-surface interactions across all
     * triangles of the geometry, considering atmospheric density, temperature, and
     * the relative velocity vector.
     *
     * @param v_rel__m_per_s The relative velocity vector of the incoming flow in the satellite's body frame [m/s].
     * @param surface_temp__K The uniform surface temperature of the satellite [K].
     * @param aero A structure containing atmospheric properties (density, temperature, particle mass, etc.).
     * @param torque__Nm Output parameter that will be populated with the calculated aerodynamic torque [Nm].
     * @param force__N Output parameter that will be populated with the calculated aerodynamic force [N].
     * @return 0 on success, or a non-zero error code on failure.
     */
    virtual int calc_aero_torque_force(const glm::vec3& v_rel__m_per_s, float surface_temp__K, AeroConditions& aero, glm::vec3& torque__Nm, glm::vec3& force__N) = 0;

    virtual ~IAeroLoadCalculator() = default;
};

} // namespace vat
