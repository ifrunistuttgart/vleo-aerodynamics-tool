#pragma once

namespace vat {

/** @brief Boltzmann constant [J/K]. */
const float BOLTZMANN_CONSTANT__J_PER_K = 1.380649e-23f;

/**
 * @brief Free-stream atmospheric state used by the GSI models.
 */
struct AeroConditions {
    /** @brief Atmospheric mass density [kg/m^3]. */
    float density__kg_per_m3;
    /** @brief Atmospheric (free-stream) temperature [K]. */
    float T_atmospheric__K;
    /** @brief Mass of one gas particle (mean molecular mass) [kg]. */
    float particle_mass__kg;
};

} // namespace vat
