#pragma once
#include <glm/glm.hpp>

class IShadingAlgorithm {
public:
	virtual ~IShadingAlgorithm() = default;
	virtual int shade_satellite(float triangle_visibility[], glm::vec3 v_rel_hat, float bounding_sphere_radius) = 0;
};
