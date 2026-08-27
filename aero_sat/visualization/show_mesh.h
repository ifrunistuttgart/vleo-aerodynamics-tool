#pragma once
#include "Isatellite_shading_data.h"
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>

namespace vat {

/**
 * Displays a satellite with triangles colored according to their visibility to the surrounding gas, based on the shading data and relative velocity.
 * @param satellite - Reference to the satellite shading data.
 * @param triangle_visibility - Vector containing the visibility values for each triangle.
 * @param v_rel__m_per_s - relative velocity vector of the satellite with respect to the surrounding gas, in the satellite's body frame.
 */
void ShowMeshWithShadingAndWind(
    ISatelliteShadingData& satellite,
    const std::vector<float>& triangle_visibility,
    const glm::vec3& v_rel__m_per_s
);

} // namespace vat
