#pragma once

namespace vat::gsi_models::projected_area_glsl {

// The C++ force per wetted area is (cos_d, 0, 0); per projected area (divided by cos_d)
// that is the constant (1, 0, 0). The integrator multiplies it by the pixel area, so
// the first force component accumulates the exposed projected area.
inline constexpr const char* force_per_projected_area = R"GLSL(
vec3 gsi_force_per_projected_area(vec3 n, vec3 v, float Tw)
{
    return vec3(1.0, 0.0, 0.0);
}
)GLSL";

} // namespace vat::gsi_models::projected_area_glsl
