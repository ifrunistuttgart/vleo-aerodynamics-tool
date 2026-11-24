#pragma once
#include <glm/glm.hpp>
#include "core.h"
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace vat {

/**
 * @brief A named uniform that a GSI model's GLSL code reads, with the value to upload.
 */
struct GlslUniform {
    /** @brief Uniform name as declared in the GLSL source (prefixed u_gsi_). */
    std::string name;
    /** @brief Value to upload. */
    std::variant<float, int> value;
};

/**
 * @brief Interface for Gas-Surface Interaction (GSI) models.
 *
 * This interface defines the contract for models that calculate aerodynamic
 * forces and torques acting on individual triangles based on physical
 * interaction theories (e.g., Sentman, Maxwell, etc.). A model knows nothing about
 * occlusion; all vectors are in the body frame.
 */
class IGSIModel {
public:
    /**
     * @brief Calculates the aerodynamic force and torque for a single triangle.
     *
     * The calculation considers the element's geometry (area, normal, centroid),
     * the gas properties (velocity, temperature, density), and the specific
     * interaction model's parameters (e.g., energy accommodation coefficients).
     * The models in this toolbox compute the force as force_per_area() * area__m2 and the
     * torque as cross(centroid__m, force), i.e. about the body-frame origin.
     *
     * @param area__m2 The area of the triangle [m^2].
     * @param normal The unit normal vector of the triangle, in the body frame.
     * @param centroid__m The centroid of the triangle in the body frame [m].
     * @param v_rel__m_per_s Velocity of the geometry relative to the atmosphere, in the body frame [m/s].
     * @param surf_temp__K The temperature of the surface [K].
     * @param aero Atmospheric conditions (density, temperature, particle mass).
     * @param aero_force__N Output: aerodynamic force on the triangle, in the body frame [N].
     * @param aero_torque__Nm Output: aerodynamic torque about the body-frame origin [Nm].
     * @return 0 on success, or a non-zero error code on failure.
     */
    virtual int calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m,
                                           const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions& aero,
                                           glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) = 0;

    /**
     * @brief Force per unit wetted area that the gas exerts on a surface element [N/m^2].
     *
     * calc_aero_force_and_torque() is this value times the element's area, so the two
     * can never disagree. It includes both the normal (pressure) and the tangential
     * (shear) part.
     *
     * @param normal The unit normal vector of the surface element, in the body frame.
     * @param v_rel__m_per_s Velocity of the geometry relative to the atmosphere, in the body frame [m/s].
     * @param surf_temp__K The temperature of the surface [K].
     * @param aero Atmospheric conditions (density, temperature, particle mass).
     * @return The force per unit wetted area, in the body frame [N/m^2].
     * @throws std::logic_error from this default implementation, for models that do not override it.
     */
    [[nodiscard]] virtual glm::vec3 force_per_area(const glm::vec3& normal, const glm::vec3& v_rel__m_per_s,
                                                   float surf_temp__K, const AeroConditions& aero) const {
        throw std::logic_error("force_per_area is not implemented for this GSI model");
    }

    /**
     * @brief GLSL source of this model's GPU implementation, for the per-pixel load calculator.
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
    [[nodiscard]] virtual std::string glsl_force_per_projected_area() const { return {}; }

    /**
     * @brief Values for the uniforms that glsl_force_per_projected_area() declares.
     *
     * Called before every evaluation, so parameter changes and new atmospheric
     * conditions take effect immediately.
     *
     * @param aero Atmospheric conditions (density, temperature, particle mass).
     * @return One entry per uniform.
     */
    [[nodiscard]] virtual std::vector<GlslUniform> glsl_uniforms(const AeroConditions& aero) const { return {}; }

    virtual ~IGSIModel() = default;

    /**
     * @brief Sets a model parameter by name, e.g. "alpha_e" or "sigma_n".
     *
     * The names are model specific. In the models of this toolbox an unknown name logs a
     * warning and is ignored, and out-of-range values are clamped by the model's setter
     * (with a warning) rather than rejected.
     *
     * @param name Parameter name.
     * @param value New value.
     */
    virtual void set_gsi_parameter(std::string name, float value) = 0;

    /**
     * @brief Reads a model parameter by name.
     *
     * @param name Parameter name, as for set_gsi_parameter().
     * @return The current value. In the models of this toolbox an unknown name logs a warning
     *         and returns 0.0f.
     */
    [[nodiscard]] virtual float get_gsi_parameter(std::string name) const = 0;
};

} // namespace vat
