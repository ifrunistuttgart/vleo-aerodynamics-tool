#include "projected_area.h"
#include "shaders/projected_area_glsl.h"
#define FMT_UNICODE 0
#include <spdlog/spdlog.h>

namespace vat::gsi_models {

int ProjectedArea::calc_aero_force_and_torque(float area__m2, const glm::vec3 &normal, const glm::vec3 &centroid__m,
                                              const glm::vec3 &v_rel__m_per_s, float surf_temp__K, AeroConditions &aero,
                                              glm::vec3 &aero_force__N, glm::vec3 &aero_torque__Nm) {
    aero_force__N = force_per_area(normal, v_rel__m_per_s, surf_temp__K, aero) * area__m2;
    aero_torque__Nm = glm::cross(centroid__m, aero_force__N);
    return 0;
}

glm::vec3 ProjectedArea::force_per_area(const glm::vec3 &normal, const glm::vec3 &v_rel__m_per_s, float surf_temp__K,
                                        const AeroConditions &aero) const {
    const float speed__m_per_s = glm::length(v_rel__m_per_s);
    if (speed__m_per_s < 1e-10f) {
        return glm::vec3(0.0f);
    }
    const float cos_d = glm::dot(v_rel__m_per_s, normal) / speed__m_per_s;
    if (cos_d <= 0.0f) {
        return glm::vec3(0.0f); // Leeward: not part of the projected area
    }
    return glm::vec3(cos_d, 0.0f, 0.0f);
}

std::string ProjectedArea::glsl_force_per_projected_area() const {
    return projected_area_glsl::force_per_projected_area;
}

std::vector<GlslUniform> ProjectedArea::glsl_uniforms(const AeroConditions &) const {
    return {};
}

void ProjectedArea::set_gsi_parameter(std::string name, float value) {
    SPDLOG_WARN("Unknown GSI parameter for gsi model ProjectedArea: {}, ignoring.", name);
}

[[nodiscard]] float ProjectedArea::get_gsi_parameter(std::string name) const {
    SPDLOG_WARN("Unknown GSI parameter for gsi model ProjectedArea: {}, ignoring.", name);
    return 0.0f;
}

} // namespace vat::gsi_models
