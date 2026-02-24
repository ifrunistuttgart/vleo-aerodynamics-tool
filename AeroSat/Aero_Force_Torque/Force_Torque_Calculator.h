#pragma once
#include "Core/IAero_calculator.h"
#include "Core/ISatellite.h"
#include "IShader.h"
#include "Core/Core.h"
#include <eigen3/Eigen/Dense>


class ForceTorqueCalculator : public IAeroCalculator {
public:
    // Konstruktor nimmt jetzt einen Zeiger auf ISatellite
    ForceTorqueCalculator(ISatellite* satellite);
    ~ForceTorqueCalculator() override;

    int calc_aero_torque_force(const Eigen::Vector3f& v_rel__m_per_s, AeroConditions aero) override;
    int change_satellite(ISatellite* satellite) override;

private:
    IShader* shader;
    ISatellite* satellite; // Zeiger statt Instanz
};
