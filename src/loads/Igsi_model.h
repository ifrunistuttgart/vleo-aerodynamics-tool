#pragma once
#include <glm/glm.hpp>
#include "core.h"
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace vat {

/**
 * A named uniform that a GSI model's GLSL code reads, with the value to upload.
 */
struct GlslUniform {
    std::string name;
    std::variant<float, int> value;
};

/**
 * Interface for Gas-Surface Interaction (GSI) models.
 *
 * This interface defines the contract for models that calculate aerodynamic
 * forces and torques acting on individual triangles based on physical
 * interaction theories (e.g., Sentman, Maxwell, etc.).
 */
class IGSIModel {
public:
    /**
     * Calculates the aerodynamic force and torque for a single triangle.
     *
     * The calculation considers the element's geometry (area, normal, centroid),
     * the gas properties (velocity, temperature, density), and the specific
     * interaction model's parameters (e.g., energy accommodation coefficients).
     *
     * @param area__m2 The area of the triangle [m^2].
     * @param normal The unit normal vector of the triangle.
     * @param centroid__m The 3D centroid position of the triangle [m].
     * @param v_rel__m_per_s The relative velocity vector of the gas flow [m/s].
     * @param surf_temp__K The temperature of the surface [K].
     * @param aero A structure containing atmospheric and interaction conditions.
     * @param aero_force__N Output parameter for the calculated aerodynamic force [N].
     * @param aero_torque__Nm Output parameter for the calculated aerodynamic torque [Nm].
     * @return 0 on success, or a non-zero error code on failure.
     */
    virtual int calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m, const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions& aero, glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) = 0;

    /**
     * Force per unit wetted area that the gas exerts on a surface element [N/m^2].
     *
     * calc_aero_force_and_torque() is this value times the element's area, so the two
     * can never disagree. It includes both the normal (pressure) and the tangential
     * (shear) part.
     *
     * @param normal The unit normal vector of the surface element.
     * @param v_rel__m_per_s The relative velocity vector of the gas flow [m/s].
     * @param surf_temp__K The temperature of the surface [K].
     * @param aero A structure containing atmospheric and interaction conditions.
     * @return The force per unit wetted area [N/m^2].
     */
    [[nodiscard]] virtual glm::vec3 force_per_area(const glm::vec3& normal, const glm::vec3& v_rel__m_per_s, float surf_temp__K, const AeroConditions& aero) const {
        throw std::logic_error("force_per_area is not implemented for this GSI model");
    }

    /**
     * GLSL source of this model's GPU implementation, for the per-pixel load calculator.
     *
     * The source must define
     *
     *     vec3 gsi_force_per_projected_area(vec3 n, vec3 v, float Tw)
     *
     * with the same conventions as force_per_area(), but per unit area projected
     * normal to the flow: force_per_area() / cos(delta), cos(delta) = dot(n, v) / |v|.
     * It is only called for dot(n, v) > 0. The host shader declares
     * `uniform float u_min_cos_delta;` before this source, for models whose projected
     * force diverges at grazing incidence. The source declares every uniform it reads
     * itself (prefixed u_gsi_) and must not contain a #version line.
     *
     * @return The GLSL source, or an empty string if the model has no GPU implementation.
     */
    [[nodiscard]] virtual std::string glsl_force_per_projected_area() const {
        return {};
    }

    /**
     * Values for the uniforms that glsl_force_per_projected_area() declares.
     *
     * Called before every evaluation, so parameter changes and new atmospheric
     * conditions take effect immediately.
     *
     * @param aero A structure containing atmospheric and interaction conditions.
     * @return One entry per uniform.
     */
    [[nodiscard]] virtual std::vector<GlslUniform> glsl_uniforms(const AeroConditions& aero) const {
        return {};
    }

    virtual ~IGSIModel() = default;

    virtual void set_gsi_parameter(std::string name, float value) = 0;
    [[nodiscard]] virtual float get_gsi_parameter(std::string name) const = 0;
};

} // namespace vat
