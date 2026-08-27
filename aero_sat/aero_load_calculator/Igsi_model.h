#pragma once
#include <glm/glm.hpp>
#include "core.h"


namespace vat {

/**
 * Interface for Gas-Surface Interaction (GSI) models.
 *
 * This interface defines the contract for models that calculate aerodynamic
 * forces and torques acting on individual triangles based on physical
 * interaction theories (e.g., Sentman, Maxwell, etc.).
 */
class IGSIModel {
public:
    /**
     * Calculates the aerodynamic force and torque for a single triangle.
     *
     * The calculation considers the triangle's area, normal and centroid,
     * the gas properties (velocity, temperature, density), and the specific
     * interaction model's parameters (e.g., energy accommodation coefficients).
     *
     * @param area__m2 The area of the triangle [m^2].
     * @param normal The unit normal vector of the triangle.
     * @param centroid__m The 3D centroid position of the triangle [m].
     * @param v_rel__m_per_s The relative velocity vector of the gas flow [m/s].
     * @param surf_temp__K The temperature of the surface [K].
     * @param aero A structure containing atmospheric and interaction conditions.
     * @param aero_force__N Output parameter for the calculated aerodynamic force [N].
     * @param aero_torque__Nm Output parameter for the calculated aerodynamic torque [Nm].
     * @return 0 on success, or a non-zero error code on failure.
     */
    virtual int calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m, const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions& aero, glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) = 0;

    virtual ~IGSIModel() = default;
};

} // namespace vat
