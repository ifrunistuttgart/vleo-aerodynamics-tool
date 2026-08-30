#pragma once

const float BOLTZMANN_CONSTANT__J_PER_K = 1.380649e-23f; // Boltzmann constant in J/K

struct AeroConditions {
    float density__kg_per_m3;
    float T_atmospheric__K;
    float particle_mass__kg;
};
