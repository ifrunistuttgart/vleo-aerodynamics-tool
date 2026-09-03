#pragma once
#include "Igsi_model_cpu.h"

namespace gsi::cpu {
    class SchaafChambre: public IGSIModelCPU {
    private:
        float m_sigma_n;
        float m_sigma_t;
    public:
        /**
        * Constructor for the Schaaf-Chambre GSI model.
        *
        * References:
        *    [1]Paul A Chambre and Samuel A Schaaf. Flow of rarefied gases. Princeton University Press, 2017
        * @param sigma_n: Scalar value of the normal accommodation coefficient, between 0 and 1
        * @param sigma_t: Scalar value of the tangential accommodation coefficient, between 0
        */
        SchaafChambre(float sigma_n, float sigma_t);
        ~SchaafChambre() override = default;

        int calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m, const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions& aero, glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) override;
        void set_gsi_parameter(std::string name, float value) override;
        [[nodiscard]] float get_gsi_parameter(std::string name) const override;

        [[nodiscard]] float get_sigma_n() const { return m_sigma_n; }
        [[nodiscard]] float get_sigma_t() const { return m_sigma_t; }
        void set_sigma_n(float sigma_n);
        void set_sigma_t(float sigma_t);
    };
}
