#pragma once
#include "Igsi_model.h"


class Sentman: public IGSIModel {
public:
    /**
    * Constructor for the Sentman GSI model.
    * 
    * References:
    *    [1] L.H.Sentman, “Free Molecule Flow Theory and Its Application to the Determination of Aerodynamic Forces,” Defense Technical Information Center, Fort Belvoir, VA, LMSC-448514, Oct. 1961.
    *    [2] F.Tuttas, C.Traub, M.Pfeiffer, and W.Fichter, “Generalized Treatment of Energy Accommodation in Gas-Surface Interactions for Satellite Aerodynamics Applications,” 2024, arXiv.doi:10.48550/ARXIV.2411.11597.
    *    [3] G.Koppenwallner, “Energy Accomodation Coefficient and Momentum Transfer Modeling,” HTG-TN-08-11, Dec. 2009.
    * @param temperature_ratio_method: Scalar value of the method to calculate the temperature ratio
    *                             1: Exact term according to [1]
    *                             2: Hyperthermal approximation according to [1]
    *                             3: Hyperthermal approximation according to [2]
    */
    Sentman(int temperature_ratio_method);
    ~Sentman() override = default;

    int calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m, const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions& aero, glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) override;


private:
    int temperature_ratio_method;
};