#define FMT_UNICODE 0
#include <spdlog/spdlog.h>
#include <cmath>
#include "schaaf_chambre.h"


SchaafChambre::SchaafChambre(float sigma_n, float sigma_t) {
    set_sigma_n(sigma_n);
    set_sigma_t(sigma_t);
}

int SchaafChambre::calc_aero_force_and_torque(float area__m2, const glm::vec3 &normal, const glm::vec3 &centroid__m,
                                     const glm::vec3 &v_rel__m_per_s, float surf_temp__K, AeroConditions &aero, glm::vec3 &aero_force__N,
                                     glm::vec3 &aero_torque__Nm) {
    // Initialize outputs
    aero_force__N = glm::vec3(0.0f);
    aero_torque__Nm = glm::vec3(0.0f);

    // Extract aerodynamic conditions
    const float density__kg_per_m3 = aero.density__kg_per_m3;
    const float temperature_i__K = aero.T_atmospheric__K;
    const float temperature_w__K = surf_temp__K;
    const float particle_mass__kg = aero.particle_mass__kg;

    glm::vec3 v_rel_inv__m_per_s = -v_rel__m_per_s; // Invert velocity to match GSIMs convention (velocity of gas relative to surface)

    const float v_rel_magnitude__m_per_s = glm::length(v_rel_inv__m_per_s);
    if (v_rel_magnitude__m_per_s < 1e-10f) {
        SPDLOG_WARN("Relative velocity zero ({} m/s), aerodynamic force and torque will be negligible.", v_rel_magnitude__m_per_s);
        return 0; // No relative velocity, no force
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

    const float cos_d = glm::dot(v_rel__m_per_s, normal) / v_rel_magnitude__m_per_s;
    const float sin_d = std::sqrt(std::max(0.0f, 1.0f - cos_d * cos_d));

    // Pre-calculate common sub-expressions for efficiency
    const float s = molecular_speed_ratio;
    const float s2 = s * s;
    const float sqrt_pi = 1.77245385f; // Precomputed value of sqrt(pi)

    const float s_cos_d = s * cos_d;
    const float s2_cos2_d = s_cos_d * s_cos_d;

    const float exp_term = std::exp(-s2_cos2_d);
    const float erf_term = 1.0f + std::erf(s_cos_d);
    const float sqrt_temp_ratio = std::sqrt(temperature_w__K / temperature_i__K);

    // --- Cp Calculation ---
    const float cp_term1 = (((2.0f - m_sigma_n) / sqrt_pi) * s_cos_d + (m_sigma_n / 2.0f) * sqrt_temp_ratio) * exp_term;
    const float cp_term2 = ((2.0f - m_sigma_n) * (s2_cos2_d + 0.5f) + (m_sigma_n / 2.0f) * sqrt_pi * s_cos_d * sqrt_temp_ratio) * erf_term;
    const float cp = (1.0f / s2) * (cp_term1 + cp_term2);

    // --- Ctau Calculation ---
    const float ctau = ((m_sigma_t * sin_d) / (s * sqrt_pi)) * (exp_term + s * sqrt_pi * cos_d * erf_term);

    const float cd = cp * cos_d + ctau * sin_d;
    // Shear acts downstream of the surface, so the tangential term subtracts from
    // lift: cd = cp*cos_d + ctau*sin_d,  cl = cp*sin_d - ctau*cos_d
    const float cl = cp * sin_d - ctau * cos_d;

    const float q = 0.5f * density__kg_per_m3 * v_rel_magnitude__m_per_s * v_rel_magnitude__m_per_s;

    const glm::vec3 fl = q * area__m2 * cl * lift_dir;
    const glm::vec3 fd = q * area__m2 * cd * drag_dir;

    aero_force__N = fl + fd;
    aero_torque__Nm = glm::cross(centroid__m, aero_force__N);

    return 0;
}

void SchaafChambre::set_gsi_parameter(std::string name, float value) {
    if (name == "sigma_n") {
        set_sigma_n(value);
    } else if (name == "sigma_t") {
        set_sigma_t(value);
    } else {
        SPDLOG_WARN("Unknown GSI parameter for gsi model SchaafChambre: {}, ignoring.", name);
    }
}

[[nodiscard]] float SchaafChambre::get_gsi_parameter(std::string name) const {
    if (name == "sigma_n") {
        return get_sigma_n();
    } else if (name == "sigma_t") {
        return get_sigma_t();
    }
    SPDLOG_WARN("Unknown GSI parameter for gsi model SchaafChambre: {}, ignoring.", name);
    return 0.0f;
}

void SchaafChambre::set_sigma_n(float sigma_n) {
    if (sigma_n < 0.0f) {
        SPDLOG_WARN("sigma_n must be positive, setting to 0.0");
        m_sigma_n = 0.0f;
    }
    else if (sigma_n > 1.0f) {
        SPDLOG_WARN("sigma_n must be less than 1.0, setting to 1.0");
        m_sigma_n = 1.0f;
    }
    else {
        m_sigma_n = sigma_n;
    }
}

void SchaafChambre::set_sigma_t(float sigma_t) {
    if (sigma_t < 0.0f) {
        SPDLOG_WARN("sigma_t must be positive, setting to 0.0");
        m_sigma_t = 0.0f;
    }
    else if (sigma_t > 1.0f) {
        SPDLOG_WARN("sigma_t must be less than 1.0, setting to 1.0");
        m_sigma_t = 1.0f;
    }
    else {
        m_sigma_t = sigma_t;
    }
}
