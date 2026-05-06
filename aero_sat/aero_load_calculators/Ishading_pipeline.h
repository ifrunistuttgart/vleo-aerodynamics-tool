#pragma once
#include <span>
#include <glm/glm.hpp>
#include "core/Isatellite_shading_data.h"    

class IShadingPipeline {
    public:
    /**
     * @param v_rel_hat
     */
    virtual int shade(std::span<float> triangle_visibility, const glm::vec3& v_rel_hat) = 0;

	virtual ~IShadingPipeline() = default;
};
