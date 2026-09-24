#pragma once

namespace vat::gsi_models::cook_glsl {

// Cook's cd and cl both carry a factor cos_d, divided out here, so the force per
// projected area stays finite at grazing incidence and needs no clamp:
//   cd / cos_d = 2 * (1 + 2/3 * cos_d * T),  cl / cos_d = 4/3 * sin_d * T.
inline constexpr const char* force_per_projected_area = R"GLSL(
uniform float u_gsi_density;
uniform float u_gsi_T_inf;
uniform float u_gsi_alpha_e;

vec3 gsi_force_per_projected_area(vec3 n, vec3 v, float Tw)
{
    float speed = length(v);
    vec3 lift_dir = vat_lift_direction(n, v);
    vec3 drag_dir = -v / speed;

    float cos_delta = dot(v, n) / speed;
    float sin_delta = sqrt(max(0.0, 1.0 - cos_delta * cos_delta));

    float temp_ratio_term = sqrt(1.0 + u_gsi_alpha_e * (Tw / u_gsi_T_inf - 1.0));
    float cd = 2.0 * (1.0 + (2.0 / 3.0) * cos_delta * temp_ratio_term);
    float cl = (4.0 / 3.0) * sin_delta * temp_ratio_term;

    float q = 0.5 * u_gsi_density * speed * speed;
    return q * cl * lift_dir + q * cd * drag_dir;
}
)GLSL";

} // namespace vat::gsi_models::cook_glsl
