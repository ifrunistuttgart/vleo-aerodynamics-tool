#include "storch.h"
#include <spdlog/spdlog.h>

Storch::Storch(const float V_w, const float sigma_n, const float sigma_t) {
    set_V_w(V_w);
    set_sigma_n(sigma_n);
    set_sigma_t(sigma_t);
}


int Storch::calc_aero_force_and_torque(float area__m2, const glm::vec3 &normal, const glm::vec3 &centroid__m,
    const glm::vec3 &v_rel__m_per_s, float surf_temp__K, AeroConditions &aero, glm::vec3 &aero_force__N,
    glm::vec3 &aero_torque__Nm) {
        // Initialize outputs
    aero_force__N = glm::vec3(0.0f);
    aero_torque__Nm = glm::vec3(0.0f);

    // Extract aerodynamic conditions
    const float density__kg_per_m3 = aero.density__kg_per_m3;
    glm::vec3 v_rel_inv__m_per_s = -v_rel__m_per_s; // Invert velocity to match GSIMs convention (velocity of gas relative to surface)

    const float v_rel_magnitude__m_per_s = glm::length(v_rel_inv__m_per_s);
    if (v_rel_magnitude__m_per_s < 1e-10f) {
        SPDLOG_WARN("Relative velocity zero ({} m/s), aerodynamic force and torque will be negligible.", v_rel_magnitude__m_per_s);
        return 0; // No relative velocity, no force
    }

    // Direction of lift: perpendicular to velocity, in the plane of normal and velocity
    // lift_dir = -normalize(cross(cross(v_flow, normal), v_flow))
    glm::vec3 lift_dir = -glm::normalize(glm::cross(glm::cross(v_rel__m_per_s, normal), v_rel__m_per_s));
    glm::vec3 drag_dir = glm::normalize(v_rel_inv__m_per_s);

    const float cos_d = glm::dot(v_rel__m_per_s, normal) / v_rel_magnitude__m_per_s;
    const float sin_d = std::sqrt(std::max(0.0f, 1.0f - cos_d * cos_d));

    // --- Cp Calculation ---
    const float cp = 2 * cos_d * ( m_sigma_n * m_V_w / v_rel_magnitude__m_per_s + (2 - m_sigma_n)*cos_d);

    // --- Ctau Calculation ---
    const float ctau = 2*m_sigma_t*sin_d*cos_d;

    const float cd = cp * cos_d + ctau * sin_d;
    const float cl = cp * sin_d + ctau * cos_d;

    const float q = 0.5f * density__kg_per_m3 * v_rel_magnitude__m_per_s * v_rel_magnitude__m_per_s;

    const glm::vec3 fl = q * area__m2 * cl * lift_dir;
    const glm::vec3 fd = q * area__m2 * cd * drag_dir;

    aero_force__N = fl + fd;
    aero_torque__Nm = glm::cross(centroid__m, aero_force__N);

    return 0;
}

void Storch::set_gsi_parameter(std::string name, const float value) {
    if (name == "sigma_n") {
        set_sigma_n(value);
    } else if (name == "sigma_t") {
        set_sigma_t(value);
    } else if (name == "V_w") {
        set_V_w(value);
    } else {
        SPDLOG_WARN("Unknown GSI parameter for gsi model Storch: {}, ignoring.", name);
    }
}

float Storch::get_gsi_parameter(std::string name) const {
    if (name == "sigma_n") {
        return get_sigma_n();
    }
    if (name == "sigma_t") {
        return get_sigma_t();
    }
    if (name == "V_w") {
        return get_V_w();
    }
    SPDLOG_WARN("Unknown GSI parameter for gsi model Storch: {}, ignoring.", name);
    return 0.0f;
}

void Storch::set_V_w(const float V_w) {
    if (V_w < 0.0f) {
        SPDLOG_WARN("V_m must be positive, setting to 0.0");
        m_V_w = 0.0f;
    }
    else {
        m_V_w = V_w;
    }
}

void Storch::set_sigma_n(const float sigma_n) {
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

void Storch::set_sigma_t(const float sigma_t) {
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
