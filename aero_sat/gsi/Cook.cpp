#define FMT_UNICODE 0
#include <spdlog/spdlog.h>
#include <cmath>
#include "Cook.h"

Cook::Cook() {
}

int Cook::calc_aero_force_and_torque(float area__m2, const glm::vec3 &normal, const glm::vec3 &centroid__m,
                                     const glm::vec3 &v_rel__m_per_s, float surf_temp__K, AeroConditions &aero, glm::vec3 &aero_force__N,
                                     glm::vec3 &aero_torque__Nm) {
    // Initialize outputs
    aero_force__N = glm::vec3(0.0f);
    aero_torque__Nm = glm::vec3(0.0f);

    // Extract aerodynamic conditions
    const float density__kg_per_m3 = aero.density__kg_per_m3;
    const float temperature_i__K = aero.temperature__K;
    const float temperature_w__K = surf_temp__K;
    const float alpha = aero.alpha_e;
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

    const float cos_delta = glm::dot(v_rel__m_per_s, normal) / v_rel_magnitude__m_per_s;
    if (cos_delta <= 0.0f) {
        return 0; // Surface not exposed to flow, hyperthermal assumption
    }
    const float sin_delta = std::sqrt(std::max(0.0f, 1.0f - cos_delta * cos_delta));
    
    const float temp_ratio_term = std::sqrt(1.0f + alpha * (temperature_w__K / temperature_i__K - 1.0f));
    const float cd = 2.0f * cos_delta * (1.0f + (2.0f / 3.0f) * cos_delta * temp_ratio_term);
    const float cl = (4.0f / 3.0f) * sin_delta * cos_delta * temp_ratio_term;
    
    const float q = 0.5f * density__kg_per_m3 * v_rel_magnitude__m_per_s * v_rel_magnitude__m_per_s;
    const glm::vec3 fl = q * area__m2 * cl * lift_dir;
    const glm::vec3 fd = q * area__m2 * cd * drag_dir;
    
    aero_force__N = fl + fd;
    aero_torque__Nm = glm::cross(centroid__m, aero_force__N);
    
    return 0;
}
