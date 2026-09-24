#pragma once

namespace vat::gsi_models::storch_glsl {

// Storch's cp and ctau both carry a factor cos_d, divided out here, so the force per
// projected area stays finite at grazing incidence and needs no clamp.
inline constexpr const char* force_per_projected_area = R"GLSL(
uniform float u_gsi_density;
uniform float u_gsi_V_w;
uniform float u_gsi_sigma_n;
uniform float u_gsi_sigma_t;

vec3 gsi_force_per_projected_area(vec3 n, vec3 v, float Tw)
{
    float speed = length(v);
    vec3 lift_dir = vat_lift_direction(n, v);
    vec3 drag_dir = -v / speed;

    float cos_d = dot(v, n) / speed;
    float sin_d = sqrt(max(0.0, 1.0 - cos_d * cos_d));

    float cp = 2.0 * (u_gsi_sigma_n * u_gsi_V_w / speed + (2.0 - u_gsi_sigma_n) * cos_d);
    float ctau = 2.0 * u_gsi_sigma_t * sin_d;

    // Shear acts downstream of the surface, so the tangential term subtracts from lift.
    float cd = cp * cos_d + ctau * sin_d;
    float cl = cp * sin_d - ctau * cos_d;

    float q = 0.5 * u_gsi_density * speed * speed;
    return q * cl * lift_dir + q * cd * drag_dir;
}
)GLSL";

} // namespace vat::gsi_models::storch_glsl
