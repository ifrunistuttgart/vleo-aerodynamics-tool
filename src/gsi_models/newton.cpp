#include "newton.h"
#include "shaders/newton_glsl.h"
#define FMT_UNICODE 0
#include <spdlog/spdlog.h>
#include <cmath>

namespace vat::gsi_models {

int Newton::calc_aero_force_and_torque(float area__m2, const glm::vec3 &normal, const glm::vec3 &centroid__m,
                                     const glm::vec3 &v_rel__m_per_s, float surf_temp__K, AeroConditions &aero, glm::vec3 &aero_force__N,
                                     glm::vec3 &aero_torque__Nm) {
    aero_force__N = force_per_area(normal, v_rel__m_per_s, surf_temp__K, aero) * area__m2;
    aero_torque__Nm = glm::cross(centroid__m, aero_force__N);
    return 0;
}

glm::vec3 Newton::force_per_area(const glm::vec3 &normal, const glm::vec3 &v_rel__m_per_s, float surf_temp__K,
                                 const AeroConditions &aero) const {
    if(glm::dot(v_rel__m_per_s, normal)< 0.0f) {
        return glm::vec3(0.0f); // No aerodynamic force if the surface is facing away from the flow
    }

    const float density__kg_per_m3 = aero.density__kg_per_m3;

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

    const float cos_d = glm::dot(v_rel__m_per_s, normal) / v_rel_magnitude__m_per_s;
    const float sin_d = std::sqrt(std::max(0.0f, 1.0f - cos_d * cos_d));

    // --- Cp Calculation ---
    const float cp = 2.0f * cos_d*cos_d;

    // --- conversion to Cd and Cl ---
    const float cd = cp * cos_d;
    const float cl = cp * sin_d;

    const float q = 0.5f * density__kg_per_m3 * v_rel_magnitude__m_per_s * v_rel_magnitude__m_per_s;

    return q * cl * lift_dir + q * cd * drag_dir;
}

std::string Newton::glsl_force_per_projected_area() const {
    return newton_glsl::force_per_projected_area;
}

std::vector<GlslUniform> Newton::glsl_uniforms(const AeroConditions &aero) const {
    return {{"u_gsi_density", aero.density__kg_per_m3}};
}

void Newton::set_gsi_parameter(std::string name, float value) {
    SPDLOG_WARN("Unknown GSI parameter for gsi model Newton: {}, ignoring.", name);
}

[[nodiscard]] float Newton::get_gsi_parameter(std::string name) const {
    SPDLOG_WARN("Unknown GSI parameter for gsi model Newton: {}, ignoring.", name);
    return 0.0f;
}

} // namespace vat::gsi_models
