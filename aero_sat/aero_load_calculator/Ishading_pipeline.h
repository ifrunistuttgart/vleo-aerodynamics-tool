#pragma once
#include <span>
#include <glm/glm.hpp>
#include "Isatellite_shading_data.h"    

/**
 * Interface for calculation of 
 */
class IShadingPipeline {
    public:
    /**
    * Calculate the shading for a set of triangles based on their visibility and relative velocity.
	* @param v_rel_hat normalized relative velocity vector of the satellite with respect to the surrounding gas, in the satellite's body frame.
    * @return 0 on success.
    */
    virtual std::vector<float> shade(const glm::vec3& v_rel_hat) = 0;

	virtual ~IShadingPipeline() = default;
};
