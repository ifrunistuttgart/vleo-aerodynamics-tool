#pragma once
#include "Igsi_model.h"

namespace vat::gsi_models {

/**
 * @brief Schaaf-Chambre gas-surface interaction model with normal and tangential accommodation
 * coefficients.
 *
 * GSI parameter names for set_gsi_parameter()/get_gsi_parameter(): "sigma_n", "sigma_t".
 *
 * References:
 *    [1] Paul A Chambre and Samuel A Schaaf. Flow of rarefied gases. Princeton University Press, 2017
 */
class SchaafChambre : public IGSIModel {
private:
    float m_sigma_n;
    float m_sigma_t;

public:
    /**
     * @brief Constructor for the Schaaf-Chambre GSI model.
     *
     * @param sigma_n Normal accommodation coefficient, in [0, 1]; out-of-range values are
     *        clamped with a warning.
     * @param sigma_t Tangential accommodation coefficient, in [0, 1]; out-of-range values are
     *        clamped with a warning.
     */
    SchaafChambre(float sigma_n, float sigma_t);
    ~SchaafChambre() override = default;

    int calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m,
                                   const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions& aero,
                                   glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) override;
    [[nodiscard]] glm::vec3 force_per_area(const glm::vec3& normal, const glm::vec3& v_rel__m_per_s, float surf_temp__K,
                                           const AeroConditions& aero) const override;
    [[nodiscard]] std::string glsl_force_per_projected_area() const override;
    [[nodiscard]] std::vector<GlslUniform> glsl_uniforms(const AeroConditions& aero) const override;
    void set_gsi_parameter(std::string name, float value) override;
    [[nodiscard]] float get_gsi_parameter(std::string name) const override;

    /** @brief Normal accommodation coefficient, in [0, 1]. */
    [[nodiscard]] float get_sigma_n() const { return m_sigma_n; }
    /** @brief Tangential accommodation coefficient, in [0, 1]. */
    [[nodiscard]] float get_sigma_t() const { return m_sigma_t; }
    /**
     * @brief Sets the normal accommodation coefficient.
     * @param sigma_n New value; values below 0 or above 1 are clamped to [0, 1] with a warning.
     */
    void set_sigma_n(float sigma_n);
    /**
     * @brief Sets the tangential accommodation coefficient.
     * @param sigma_t New value; values below 0 or above 1 are clamped to [0, 1] with a warning.
     */
    void set_sigma_t(float sigma_t);
};

} // namespace vat::gsi_models
