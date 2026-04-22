#pragma once
#include "Core/ISatellite_shading_data.h"

void ShowMeshWithShadingAndWind(
    ISatelliteShadingData& satellite,
    const std::vector<float>& triangle_visibility,
    const glm::vec3& v_rel__m_per_s);