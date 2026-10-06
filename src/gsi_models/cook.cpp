#define FMT_UNICODE 0
#include <spdlog/spdlog.h>
#include <cmath>
#include "cook.h"
#include "shaders/common_glsl.h"
#include "shaders/cook_glsl.h"

namespace vat::gsi_models {

Cook::Cook(const float alpha_e) {
    set_alpha_e(alpha_e);
}

int Cook::calc_aero_force_and_torque(float area__m2, const glm::vec3 &normal, const glm::vec3 &centroid__m,
                                     const glm::vec3 &v_rel__m_per_s, float surf_temp__K, AeroConditions &aero, glm::vec3 &aero_force__N,
                                     glm::vec3 &aero_torque__Nm) {
    aero_force__N = force_per_area(normal, v_rel__m_per_s, surf_temp__K, aero) * area__m2;
    aero_torque__Nm = glm::cross(centroid__m, aero_force__N);
    return 0;
}

glm::vec3 Cook::force_per_area(const glm::vec3 &normal, const glm::vec3 &v_rel__m_per_s, float surf_temp__K,
                                 const AeroConditions &aero) const {
    // Extract aerodynamic conditions
    const float density__kg_per_m3 = aero.density__kg_per_m3;
    const float temperature_i__K = aero.T_atmospheric__K;
    const float temperature_w__K = surf_temp__K;
    glm::vec3 v_rel_inv__m_per_s = -v_rel__m_per_s; // Invert velocity to match GSIMs convention (velocity of gas relative to surface)
    
    const float v_rel_magnitude__m_per_s = glm::length(v_rel_inv__m_per_s);
    if (v_rel_magnitude__m_per_s < 1e-10f) {
        SPDLOG_WARN("Relative velocity zero ({} m/s), aerodynamic force and torque will be negligible.", v_rel_magnitude__m_per_s);
        return glm::vec3(0.0f); // No relative velocity, no force
    }

    // Direction of lift: perpendicular to velocity, in the plane of normal and velocity
    // lift_dir = -normalize(cross(cross(v_flow, normal), v_flow))
    glm::vec3 lift_dir(0.0f);
    const glm::vec3 flow_normal_cross = glm::cross(v_rel__m_per_s, normal);
    if (glm::length(flow_normal_cross) > 1e-10f) {
        lift_dir = -glm::normalize(glm::cross(flow_normal_cross, v_rel__m_per_s));
    }
    glm::vec3 drag_dir = glm::normalize(v_rel_inv__m_per_s);

    const float cos_delta = glm::dot(v_rel__m_per_s, normal) / v_rel_magnitude__m_per_s;
    if (cos_delta <= 0.0f) {
        return glm::vec3(0.0f); // Surface not exposed to flow, hyperthermal assumption
    }
    const float sin_delta = std::sqrt(std::max(0.0f, 1.0f - cos_delta * cos_delta));
    
    const float temp_ratio_term = std::sqrt(1.0f + m_alpha_e * (temperature_w__K / temperature_i__K - 1.0f));
    const float cd = 2.0f * cos_delta * (1.0f + (2.0f / 3.0f) * cos_delta * temp_ratio_term);
    const float cl = (4.0f / 3.0f) * sin_delta * cos_delta * temp_ratio_term;
    
    const float q = 0.5f * density__kg_per_m3 * v_rel_magnitude__m_per_s * v_rel_magnitude__m_per_s;
    return q * cl * lift_dir + q * cd * drag_dir;
}

std::string Cook::glsl_force_per_projected_area() const {
    return std::string(common_glsl::helpers) + cook_glsl::force_per_projected_area;
}

std::vector<GlslUniform> Cook::glsl_uniforms(const AeroConditions &aero) const {
    return {
        {"u_gsi_density", aero.density__kg_per_m3},
        {"u_gsi_T_inf", aero.T_atmospheric__K},
        {"u_gsi_alpha_e", m_alpha_e},
    };
}

void Cook::set_gsi_parameter(std::string name, float value) {
    if (name == "alpha_e") {
        set_alpha_e(value);
    } else {
        SPDLOG_WARN("Unknown GSI parameter for gsi model Cook: {}, ignoring.", name);
    }
}

[[nodiscard]] float Cook::get_gsi_parameter(std::string name) const {
    if (name == "alpha_e") {
        return this->get_alpha_e();
    }
    SPDLOG_WARN("Unknown GSI parameter for gsi model Cook: {}, ignoring.", name);
    return 0.0f;
}

void Cook::set_alpha_e(float alpha_e) {
    if (alpha_e < 0.0f) {
        SPDLOG_WARN("alpha_e must be positive, setting to 0.0");
        m_alpha_e = 0.0f;
    }
    else if (alpha_e > 1.0f) {
        SPDLOG_WARN("alpha_e must be less than 1.0, setting to 1.0");
        m_alpha_e = 1.0f;
    }
    else {
        m_alpha_e = alpha_e;
    }
}

} // namespace vat::gsi_models
