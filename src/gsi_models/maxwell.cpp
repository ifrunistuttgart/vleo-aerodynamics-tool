#define FMT_UNICODE 0
#include <spdlog/spdlog.h>
#include <cmath>
#include "maxwell.h"
#include "shaders/common_glsl.h"
#include "shaders/maxwell_glsl.h"

namespace vat::gsi_models {

Maxwell::Maxwell(const float alpha_e){
    set_alpha_e(alpha_e);
}

int Maxwell::calc_aero_force_and_torque(float area__m2, const glm::vec3 &normal, const glm::vec3 &centroid__m,
                                     const glm::vec3 &v_rel__m_per_s, float surf_temp__K, AeroConditions &aero, glm::vec3 &aero_force__N,
                                     glm::vec3 &aero_torque__Nm) {
    aero_force__N = force_per_area(normal, v_rel__m_per_s, surf_temp__K, aero) * area__m2;
    aero_torque__Nm = glm::cross(centroid__m, aero_force__N);
    return 0;
}

glm::vec3 Maxwell::force_per_area(const glm::vec3 &normal, const glm::vec3 &v_rel__m_per_s, float surf_temp__K,
                                 const AeroConditions &aero) const {
    if(glm::dot(v_rel__m_per_s, normal)< 0.0f) {
        return glm::vec3(0.0f); // No aerodynamic force if the surface is facing away from the flow
    }
    // Extract aerodynamic conditions
    const float density__kg_per_m3 = aero.density__kg_per_m3;
    const float temperature_i__K = aero.T_atmospheric__K;
    const float temperature_w__K = surf_temp__K;
    const float epsilon = 1 - m_alpha_e;
    const float particle_mass__kg = aero.particle_mass__kg;

    glm::vec3 v_rel_inv__m_per_s = -v_rel__m_per_s; // Invert velocity to match GSIMs convention (velocity of gas relative to surface)

    const float v_rel_magnitude__m_per_s = glm::length(v_rel_inv__m_per_s);
    if (v_rel_magnitude__m_per_s < 1e-10f) {
        SPDLOG_WARN("Relative velocity zero ({} m/s), aerodynamic force and torque will be negligible.", v_rel_magnitude__m_per_s);
        return glm::vec3(0.0f); // No relative velocity, no force
    }

    // Most probable thermal velocity of the gas
    const float thermal_velocity__m_per_s = std::sqrt(2.0f * BOLTZMANN_CONSTANT__J_PER_K * temperature_i__K / particle_mass__kg);

    const float molecular_speed_ratio = v_rel_magnitude__m_per_s / thermal_velocity__m_per_s;

    // Direction of lift: perpendicular to velocity, in the plane of normal and velocity
    // lift_dir = -normalize(cross(cross(v_flow, normal), v_flow))
    glm::vec3 lift_dir(0.0f);
    const glm::vec3 flow_normal_cross = glm::cross(v_rel__m_per_s, normal);
    if (glm::length(flow_normal_cross) > 1e-10f) {
        lift_dir = -glm::normalize(glm::cross(flow_normal_cross, v_rel__m_per_s));
    }
    glm::vec3 drag_dir = glm::normalize(v_rel_inv__m_per_s);

    const float sin_alpha = glm::dot(v_rel__m_per_s, normal) / v_rel_magnitude__m_per_s;
    const float cos_alpha = std::sqrt(std::max(0.0f, 1.0f - sin_alpha * sin_alpha));

    // Pre-calculate common sub-expressions for efficiency
    const float s = molecular_speed_ratio;
    const float s2 = s * s;
    const float sin2_alpha = sin_alpha * sin_alpha;
    const float cos_2alpha = cos_alpha * cos_alpha - sin2_alpha; // Double angle: cos(2*alpha)
    const float sqrt_pi = 1.77245385f; // Precomputed value of sqrt(pi)

    const float exp_term = std::exp(-s2 * sin2_alpha);
    const float erf_term = std::erf(s * sin_alpha);
    const float sqrt_temp_ratio = std::sqrt(temperature_w__K / temperature_i__K);

    // --- Lift Coefficient (Cl) Implementation ---
    const float cl_term1 = (4.0f * epsilon) / (sqrt_pi * s) * sin_alpha * cos_alpha * exp_term;
    const float cl_term2 = (cos_alpha / s2) * (1.0f + epsilon * (1.0f + 4.0f * s2 * sin2_alpha)) * erf_term;
    const float cl_term3 = ((1.0f - epsilon) / s) * sqrt_pi * sin_alpha * cos_alpha * sqrt_temp_ratio;

    const float cl = cl_term1 + cl_term2 + cl_term3;

    // --- Drag Coefficient (Cd) Implementation ---
    const float cd_term1 = 2.0f * ((1.0f - epsilon * cos_2alpha) / (sqrt_pi * s)) * exp_term;
    const float cd_term2 = (sin_alpha / s2) * (1.0f + 2.0f * s2 + epsilon * (1.0f - 2.0f * s2 * cos_2alpha)) * erf_term;
    const float cd_term3 = ((1.0f - epsilon) / s) * sqrt_pi * sin2_alpha * sqrt_temp_ratio;

    const float cd = cd_term1 + cd_term2 + cd_term3;

    const float q = 0.5f * density__kg_per_m3 * v_rel_magnitude__m_per_s * v_rel_magnitude__m_per_s;

    return q * cl * lift_dir + q * cd * drag_dir;
}

std::string Maxwell::glsl_force_per_projected_area() const {
    return std::string(common_glsl::helpers) + maxwell_glsl::force_per_projected_area;
}

std::vector<GlslUniform> Maxwell::glsl_uniforms(const AeroConditions &aero) const {
    return {
        {"u_gsi_density", aero.density__kg_per_m3},
        {"u_gsi_T_inf", aero.T_atmospheric__K},
        {"u_gsi_particle_mass", aero.particle_mass__kg},
        {"u_gsi_alpha_e", m_alpha_e},
    };
}

void Maxwell::set_gsi_parameter(std::string name, float value) {
    if (name == "alpha_e") {
        set_alpha_e(value);
    } else {
        SPDLOG_WARN("Unknown GSI parameter for gsi model Maxwell: {}, ignoring.", name);
    }
}

[[nodiscard]] float Maxwell::get_gsi_parameter(std::string name) const {
    if (name == "alpha_e") {
        return this->get_alpha_e();
    }
    SPDLOG_WARN("Unknown GSI parameter for gsi model Maxwell: {}, ignoring.", name);
    return 0.0f;
}

void Maxwell::set_alpha_e(float alpha_e) {
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
