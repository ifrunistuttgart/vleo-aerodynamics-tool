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
	* @param triangle_visibility Span containing the visibility values for each triangle. they can range from 0 to 1, where 0 means fully shaded and 1 means fully visible.
	* @param v_rel_hat normalized relative velocity vector of the satellite with respect to the surrounding gas, in the satellite's body frame.
    * @return 0 on success.
    */
    virtual int shade(std::span<float> triangle_visibility, const glm::vec3& v_rel_hat) = 0;

	virtual ~IShadingPipeline() = default;
};
