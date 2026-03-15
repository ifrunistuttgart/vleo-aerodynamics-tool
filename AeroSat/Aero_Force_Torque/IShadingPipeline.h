#pragma once
#include <span>
#include <eigen3/Eigen/Dense>
#include "Core/ISatellite_shading_data.h"    

class IShadingPipeline {
    public:
    /**
     * @param v_rel_hat
     */
    virtual int shade(std::span<float> triangle_visibility, const Eigen::Vector3f& v_rel_hat) = 0;

	virtual ~IShadingPipeline() = default;
};
