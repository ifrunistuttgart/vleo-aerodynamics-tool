#pragma once

namespace vat::gsi_models::newton_glsl {

// Newton's cd = cp*cos_d and cl = cp*sin_d with cp = 2*cos_d^2 combine to a purely
// normal force per wetted area, -q*cp*n. Per projected area (divided by cos_d) that is
// -2*q*cos_d*n, which stays finite at grazing incidence and needs no clamp.
inline constexpr const char* force_per_projected_area = R"GLSL(
uniform float u_gsi_density;

vec3 gsi_force_per_projected_area(vec3 n, vec3 v, float Tw)
{
    float speed = length(v);
    float cos_d = dot(v, n) / speed;
    float q = 0.5 * u_gsi_density * speed * speed;
    return -2.0 * q * cos_d * n;
}
)GLSL";

} // namespace vat::gsi_models::newton_glsl
