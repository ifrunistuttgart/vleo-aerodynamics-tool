#pragma once
#include "Aero_Force_Torque/IShadingPipeline.h"
#include "Core/ISatellite.h"
#include <eigen3/Eigen/Dense>
#include "src/IShading_Algorithm.h"

class ShadingPipeline: public IShadingPipeline {
public:
    IShadingAlgorithm& m_algorithm;
    ISatellite& m_satellite;

public:
    ShadingPipeline(ISatellite& satellite, IShadingAlgorithm& algorithm);
	~ShadingPipeline() override = default;
    int set_satellite(ISatellite& satellite) override;
    int shade(const Eigen::Vector3f& v_rel_hat) override;

};