#pragma once
#include "Core.h"
#include "ISatellite.h"
#include <eigen3/Eigen/Dense>

class IAeroCalculator {
public:
    /**
     * @param v_rel__m_per_s
     * @param aero
     */
    virtual int calc_aero_torque_force(const Eigen::Vector3f& v_rel__m_per_s, AeroConditions aero) = 0;

    /**
     * @param satellite
     */
    virtual int change_satellite(ISatellite* satellite) = 0;

    virtual ~IAeroCalculator() = default;
};