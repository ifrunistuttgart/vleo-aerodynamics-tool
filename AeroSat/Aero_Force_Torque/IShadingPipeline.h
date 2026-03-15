#pragma once
#include <eigen3/Eigen/Dense>
#include "Core/ISatellite_shading_data.h"    
//#include "IshadingAlgorithm"

class IShadingPipeline {
    public: 
    ///**
    // * @param satellite
    // */
    //virtual int set_satellite(ISatelliteShadingData& satellite) = 0;
    //
    /**
     * @param v_rel_hat
     */
    virtual int shade(float* triangle_visibility, const Eigen::Vector3f& v_rel_hat) = 0;

	virtual ~IShadingPipeline() = default;
};
