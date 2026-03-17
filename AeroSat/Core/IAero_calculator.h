#pragma once
#include "Core.h"
#include <eigen3/Eigen/Dense>

class IAeroCalculator {
public:
    /**
     * @param v_rel__m_per_s
     * @param aero
     */
    virtual int calc_aero_torque_force(const Eigen::Vector3f& v_rel__m_per_s, AeroConditions aero, Eigen::Vector3f& torque__Nm, Eigen::Vector3f& force__N) = 0;

    virtual ~IAeroCalculator() = default;
};