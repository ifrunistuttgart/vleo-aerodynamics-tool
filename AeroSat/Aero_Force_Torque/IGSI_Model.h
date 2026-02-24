#pragma once
#include <eigen3/Eigen/Dense>
#include "Core/Core.h"

class IGSIModel {
    public:

    /**
    * Calculate aerodynamic force and torque for a single surface element
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
    virtual int calc_aero_force_and_torque(float area__m2, const Eigen::Vector3f& normal, const Eigen::Vector3f& centroid__m, const Eigen::Vector3f& v_rel__m_per_s, float surf_temp__K, AeroConditions aero, Eigen::Vector3f& aero_force__N, Eigen::Vector3f& aero_torque__Nm) = 0;

	virtual ~IGSIModel() = default;
};