#pragma once
#include "Igsi_model.h"

class Storch: public IGSIModel {
private:
    float m_V_w;
    float m_sigma_n;
    float m_sigma_t;
public:
    /**
    * Constructor for the Storch GSI model.
    *
    * References:
    *    [1] J. A. Storch. Aerodynamic Disturbances on Spacecraft in Free-Molecular Flow:. Technical report, Defense Technical Information Center, Fort Belvoir, VA, October 2002.
    * @param V_w: average normal velocity of diffusely reflected molecules
    * @param sigma_n: Scalar value of the normal accommodation coefficient, between 0 and 1
    * @param sigma_t: Scalar value of the tangential accommodation coefficient, between 0
    */
    Storch(float V_w, float sigma_n, float sigma_t);
    ~Storch() override = default;

    int calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m, const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions& aero, glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) override;
    void set_gsi_parameter(std::string name, float value) override;
    [[nodiscard]] float get_gsi_parameter(std::string name) const override;

    [[nodiscard]] float get_V_w() const { return m_V_w; }
    void set_V_w(float V_w);
    [[nodiscard]] float get_sigma_n() const { return m_sigma_n; }
    [[nodiscard]] float get_sigma_t() const { return m_sigma_t; }
    void set_sigma_n(float sigma_n);
    void set_sigma_t(float sigma_t);
};
