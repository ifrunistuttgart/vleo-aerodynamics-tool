#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <glm/glm.hpp>
#include "sentman.h"


Sentman::Sentman(int temperature_ratio_method,float alpha_e){
    if (temperature_ratio_method < 1 || temperature_ratio_method > 3) {
        SPDLOG_ERROR("Invalid temperature_ratio_method: must be 1, 2, or 3 (value={})", temperature_ratio_method);
        throw std::invalid_argument(
            "Invalid temperature_ratio_method: must be 1, 2, or 3"
        );
    }

    m_temperature_ratio_method = temperature_ratio_method;
    set_alpha_e(alpha_e);

}


int Sentman::calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m, const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions& aero, glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm){
    // Initialize outputs
    aero_force__N = glm::vec3(0.0f);
    aero_torque__Nm = glm::vec3(0.0f);

    // Extract aerodynamic conditions
    const float density__kg_per_m3 = aero.density__kg_per_m3;
    const float temperature_i__K = aero.T_atmospheric__K;
    const float temperature_w__K = surf_temp__K;
    const float particle_mass__kg = aero.particle_mass__kg;
    glm::vec3 v_rel_inv__m_per_s = -v_rel__m_per_s; // Invert velocity to match Sentman's convention (velocity of gas relative to surface)

    // Velocity magnitude
    const float v_rel_magnitude__m_per_s = glm::length(v_rel_inv__m_per_s);
    if (v_rel_magnitude__m_per_s < 1e-10f) {
        SPDLOG_WARN("Relative velocity zero ({} m/s), aerodynamic force and torque will be negligible.", v_rel_magnitude__m_per_s);
        return 0; // No relative velocity, no force
    }

    // Most probable thermal velocity of the gas
    const float thermal_velocity__m_per_s = std::sqrt(2.0f * BOLTZMANN_CONSTANT__J_PER_K * temperature_i__K / particle_mass__kg);

    // Molecular speed ratio
    const float molecular_speed_ratio = v_rel_magnitude__m_per_s / thermal_velocity__m_per_s;

    const float cos_delta = glm::dot(v_rel__m_per_s, normal) / v_rel_magnitude__m_per_s;
    const float s_cos_delta = molecular_speed_ratio * cos_delta;

    // Intermediate values
    const float inv_sqrt_pi = 1.0f / std::sqrt(std::numbers::pi_v<float>);
    const float exp_term = std::exp(-s_cos_delta * s_cos_delta);
    const float erfc_term = std::erfc(-s_cos_delta);

    const float g1 = s_cos_delta * inv_sqrt_pi * exp_term + (0.5f + s_cos_delta * s_cos_delta) * erfc_term;
    const float g2 = inv_sqrt_pi * exp_term + s_cos_delta * erfc_term;

    // Temperature ratio calculation based on method
    float temperature_ratio;
    switch (m_temperature_ratio_method) {
    case 1: {
        // Exact term according to Sentman
        const float enum_val = s_cos_delta * erfc_term;
        const float denom = inv_sqrt_pi * exp_term + enum_val;
        temperature_ratio = m_alpha_e * (2.0f * BOLTZMANN_CONSTANT__J_PER_K * temperature_w__K) / (particle_mass__kg * v_rel_magnitude__m_per_s * v_rel_magnitude__m_per_s) * molecular_speed_ratio * molecular_speed_ratio +
            (1.0f - m_alpha_e) * (1.0f + molecular_speed_ratio * molecular_speed_ratio / 2.0f + 0.25f * enum_val / denom);
        break;
    }
    case 2: {
        // Hyperthermal approximation according to Tuttas
        temperature_ratio = molecular_speed_ratio * molecular_speed_ratio / 2.0f * (1.0f + m_alpha_e * ((4.0f * BOLTZMANN_CONSTANT__J_PER_K * temperature_w__K) / (particle_mass__kg * v_rel_magnitude__m_per_s * v_rel_magnitude__m_per_s) - 1.0f)) +
            1.25f * (1.0f - m_alpha_e);
        break;
    }
    case 3: {
        // Hyperthermal approximation according to Koppenwallner
        temperature_ratio = molecular_speed_ratio * molecular_speed_ratio / 2.0f * (1.0f + m_alpha_e * ((4.0f * BOLTZMANN_CONSTANT__J_PER_K * temperature_w__K) / (particle_mass__kg * v_rel_magnitude__m_per_s * v_rel_magnitude__m_per_s) - 1.0f));
        break;
    }
    default:
        return -1; // Should not reach here
    }

    // Momentum flux calculation
    const float sqrt_temperature_ratio = std::sqrt(temperature_ratio);
    const float pressure_coeff = density__kg_per_m3 / 2.0f * thermal_velocity__m_per_s * thermal_velocity__m_per_s;
    const float sqrt_pi_half = std::sqrt(std::numbers::pi_v<float>) / 2.0f;

    const float term_1 = -(g1 + sqrt_pi_half * sqrt_temperature_ratio * g2);
    const float term_2 = molecular_speed_ratio * g2;

    // Normalized velocity
    const glm::vec3 v_rel_normalized = v_rel_inv__m_per_s / v_rel_magnitude__m_per_s;

    // Pressure vector
    const glm::vec3 pressure__n_per_m2 = pressure_coeff * (term_1 * normal + term_2 * (v_rel_normalized + cos_delta * normal));

    // Force = pressure * area
    aero_force__N = pressure__n_per_m2 * area__m2;

    // Torque = centroid x force
    aero_torque__Nm = glm::cross(centroid__m, aero_force__N);

    return 0; // Success

}
void Sentman::set_gsi_parameter(std::string name, float value) {
    if (name == "alpha_e") {
        set_alpha_e(value);
    } else {
        SPDLOG_WARN("Unknown GSI parameter for gsi model Sentman: {}, ignoring.", name);
    }
}

[[nodiscard]] float Sentman::get_gsi_parameter(std::string name) const {
    if (name == "alpha_e") {
        return get_alpha_e();
    }
    SPDLOG_WARN("Unknown GSI parameter for gsi model Sentman: {}, ignoring.", name);
    return 0.0f;
}

void Sentman::set_alpha_e(float alpha_e) {
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

