#pragma once
#include "Igsi_model_cpu.h"

namespace gsi::cpu {
    class Maxwell: public IGSIModelCPU {
    private:
        float m_alpha_e;
    public:
        /**
        * Constructor for the Maxwell GSI model.
        *
        * References:
        *    [1] G A Bird. Molecular Gas Dynamics And The Direct Simulation Of Gas Flows. Oxford University Press, May 1994.
        * @param alpha_e: Scalar value of the energy accommodation coefficient, between 0 and 1
        */
        Maxwell(float alpha_e);
        ~Maxwell() override = default;

        int calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m, const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions& aero, glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) override;
        void set_gsi_parameter(std::string name, float value) override;
        [[nodiscard]] float get_gsi_parameter(std::string name) const override;

        [[nodiscard]] float get_alpha_e() const {return m_alpha_e;};
        void set_alpha_e(float alpha_e);

    };
}