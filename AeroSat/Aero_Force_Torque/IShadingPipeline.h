#pragma once
#include <eigen3/Eigen/Dense>
#include "Core/ISatellite.h"    
//#include "IshadingAlgorithm"

class IShadingPipeline {
    public: 
    /**
     * @param satellite
     */
    virtual int set_satellite(ISatellite& satellite) = 0;
    
    /**
     * @param v_rel_hat
     */
    virtual int shade(const Eigen::Vector3f& v_rel_hat) = 0;

	virtual ~IShadingPipeline() = default;
};
