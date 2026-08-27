#pragma once

namespace vat {

struct AeroConditions {
    float density__kg_per_m3;
    float temperature__K;
    float particle_mass__kg;
    float alpha_e;
};

} // namespace vat
