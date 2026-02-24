#pragma once
#include <eigen3/Eigen/Dense>
//#include "IshadingAlgorithm"

class IShader {
    public: 
    
    /**
     * @param algorithm
     */
    /*void set_shading_algorithm(IShading_Algorithm algorithm);*/
    
    /**
     * @param satellite
     */
    virtual int set_satellite(ISatellite satellite) = 0;
    
    /**
     * @param v_rel_hat
     */
    virtual int shade(const Eigen::Vector3f& v_rel_hat) = 0;

	virtual ~IShader() = default;
};
