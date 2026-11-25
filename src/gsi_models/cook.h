#pragma once
#include "Igsi_model.h"

namespace vat::gsi_models {

/**
 * @brief Cook gas-surface interaction model with an energy accommodation coefficient.
 *
 * GSI parameter name for set_gsi_parameter()/get_gsi_parameter(): "alpha_e".
 *
 * References:
 *    [1] G. E. Cook. Satellite drag coefficients. Planetary and Space Science, 13(10):929–946, October 1965
 */
class Cook : public IGSIModel {
private:
    float m_alpha_e;

public:
    /**
     * @brief Constructor for the Cook GSI model.
     *
     * @param alpha_e Energy accommodation coefficient, in [0, 1]; out-of-range values are
     *        clamped with a warning (see set_alpha_e()).
     */
    Cook(float alpha_e);
    ~Cook() override = default;

    int calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m,
                                   const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions& aero,
                                   glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) override;
    [[nodiscard]] glm::vec3 force_per_area(const glm::vec3& normal, const glm::vec3& v_rel__m_per_s, float surf_temp__K,
                                           const AeroConditions& aero) const override;
    [[nodiscard]] std::string glsl_force_per_projected_area() const override;
    [[nodiscard]] std::vector<GlslUniform> glsl_uniforms(const AeroConditions& aero) const override;
    void set_gsi_parameter(std::string name, float value) override;
    [[nodiscard]] float get_gsi_parameter(std::string name) const override;

    /** @brief Energy accommodation coefficient, in [0, 1]. */
    [[nodiscard]] float get_alpha_e() const { return m_alpha_e; }
    /**
     * @brief Sets the energy accommodation coefficient.
     * @param alpha_e New value; values below 0 or above 1 are clamped to [0, 1] with a warning.
     */
    void set_alpha_e(float alpha_e);
};

} // namespace vat::gsi_models
