#pragma once
#include <span>
#include <cstdint>
#include <glm/glm.hpp>

class IShadingAlgorithm {
public:
	virtual ~IShadingAlgorithm() = default;
	virtual int set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) = 0;
	virtual int shade_satellite(std::span<float> triangle_visibility, glm::vec3 v_rel_hat, float bounding_sphere_radius) = 0;
};
