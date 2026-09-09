#pragma once
#include "Igsi_model.h"

class Cook: public IGSIModel {
private:
    float m_alpha_e;
public:
    /**
    * Constructor for the Cook GSI model.
    *
    * References:
    *    [1] G. E. Cook. Satellite drag coefficients. Planetary and Space Science, 13(10):929–946, October 1965
    * @param alpha_e: Scalar value of the energy accommodation coefficient, between 0 and 1
    */
    Cook(float alpha_e);
    ~Cook() override = default;

    int calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m, const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions& aero, glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) override;
    void set_gsi_parameter(std::string name, float value) override;
    [[nodiscard]] float get_gsi_parameter(std::string name) const override;

    [[nodiscard]] float get_alpha_e() const { return m_alpha_e; }
    void set_alpha_e(float alpha_e);
};
