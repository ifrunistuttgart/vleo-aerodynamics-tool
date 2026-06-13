#pragma once
#include <glm/glm.hpp>
#include "core.h"


/*
 * Interface for calculating aerodynamic forces and torques on satellite surfaces using a gsi model of choice.
 * This class provides a common interface for different GSI (Gas Surface Interaction) models.
 */
class IGSIModel {
public:

    /**
    * Calculate aerodynamic force and torque for a single surface element
    * 
    * @param area__m2 Area of surface element
    * @param normal Surface normal vector
    * @param centroid__m Surface centroid
    * @param v_rel__m_per_s Relative velocity vector
    * @param surf_temp__K Surface temperature
    * @param aero Aerodynamic conditions
    * #TODO add the literatre for the reference
    * @param aero_force__N Output: Total aerodynamic force [3]
    * @param aero_torque__Nm Output: Total aerodynamic torque [3]
    * @return 0 on success
    */
    virtual int calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m, const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions& aero, glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) = 0;

    virtual ~IGSIModel() = default;
};