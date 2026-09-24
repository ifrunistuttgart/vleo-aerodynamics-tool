#pragma once

namespace vat::gsi_models::schaaf_chambre_glsl {

// Same formula as SchaafChambre::force_per_area(), divided by cos(delta). The thermal
// terms stay non-zero at grazing incidence, so the division is clamped.
inline constexpr const char* force_per_projected_area = R"GLSL(
uniform float u_gsi_density;
uniform float u_gsi_T_inf;
uniform float u_gsi_particle_mass;
uniform float u_gsi_sigma_n;
uniform float u_gsi_sigma_t;

vec3 gsi_force_per_projected_area(vec3 n, vec3 v, float Tw)
{
    float speed = length(v);

    // Most probable thermal velocity of the gas and molecular speed ratio
    float thermal_velocity = sqrt(2.0 * VAT_BOLTZMANN_CONSTANT * u_gsi_T_inf / u_gsi_particle_mass);
    float s = speed / thermal_velocity;

    vec3 lift_dir = vat_lift_direction(n, v);
    vec3 drag_dir = -v / speed;

    float cos_d = dot(v, n) / speed;
    float sin_d = sqrt(max(0.0, 1.0 - cos_d * cos_d));

    float s2 = s * s;
    float s_cos_d = s * cos_d;
    float s2_cos2_d = s_cos_d * s_cos_d;
    float exp_term = exp(-s2_cos2_d);
    float erf_term = 1.0 + vat_erf(s_cos_d);
    float sqrt_temp_ratio = sqrt(Tw / u_gsi_T_inf);

    float cp_term1 = (((2.0 - u_gsi_sigma_n) / VAT_SQRT_PI) * s_cos_d + (u_gsi_sigma_n / 2.0) * sqrt_temp_ratio) * exp_term;
    float cp_term2 = ((2.0 - u_gsi_sigma_n) * (s2_cos2_d + 0.5) + (u_gsi_sigma_n / 2.0) * VAT_SQRT_PI * s_cos_d * sqrt_temp_ratio) * erf_term;
    float cp = (1.0 / s2) * (cp_term1 + cp_term2);

    float ctau = ((u_gsi_sigma_t * sin_d) / (s * VAT_SQRT_PI)) * (exp_term + s * VAT_SQRT_PI * cos_d * erf_term);

    // Shear acts downstream of the surface, so the tangential term subtracts from lift.
    float cd = cp * cos_d + ctau * sin_d;
    float cl = cp * sin_d - ctau * cos_d;

    float q = 0.5 * u_gsi_density * speed * speed;
    return (q * cl * lift_dir + q * cd * drag_dir) / max(cos_d, u_min_cos_delta);
}
)GLSL";

} // namespace vat::gsi_models::schaaf_chambre_glsl
