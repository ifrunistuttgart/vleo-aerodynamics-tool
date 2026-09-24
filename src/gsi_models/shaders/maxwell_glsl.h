#pragma once

namespace vat::gsi_models::maxwell_glsl {

// Same formula as Maxwell::force_per_area(), divided by cos(delta). The thermal
// terms (exp_term) stay non-zero at grazing incidence, so the division is clamped.
inline constexpr const char* force_per_projected_area = R"GLSL(
uniform float u_gsi_density;
uniform float u_gsi_T_inf;
uniform float u_gsi_particle_mass;
uniform float u_gsi_alpha_e;

vec3 gsi_force_per_projected_area(vec3 n, vec3 v, float Tw)
{
    float speed = length(v);
    float epsilon = 1.0 - u_gsi_alpha_e;

    // Most probable thermal velocity of the gas and molecular speed ratio
    float thermal_velocity = sqrt(2.0 * VAT_BOLTZMANN_CONSTANT * u_gsi_T_inf / u_gsi_particle_mass);
    float s = speed / thermal_velocity;

    vec3 lift_dir = vat_lift_direction(n, v);
    vec3 drag_dir = -v / speed;

    float sin_alpha = dot(v, n) / speed;
    float cos_alpha = sqrt(max(0.0, 1.0 - sin_alpha * sin_alpha));

    float s2 = s * s;
    float sin2_alpha = sin_alpha * sin_alpha;
    float cos_2alpha = cos_alpha * cos_alpha - sin2_alpha;
    float exp_term = exp(-s2 * sin2_alpha);
    float erf_term = vat_erf(s * sin_alpha);
    float sqrt_temp_ratio = sqrt(Tw / u_gsi_T_inf);

    float cl_term1 = (4.0 * epsilon) / (VAT_SQRT_PI * s) * sin_alpha * cos_alpha * exp_term;
    float cl_term2 = (cos_alpha / s2) * (1.0 + epsilon * (1.0 + 4.0 * s2 * sin2_alpha)) * erf_term;
    float cl_term3 = ((1.0 - epsilon) / s) * VAT_SQRT_PI * sin_alpha * cos_alpha * sqrt_temp_ratio;
    float cl = cl_term1 + cl_term2 + cl_term3;

    float cd_term1 = 2.0 * ((1.0 - epsilon * cos_2alpha) / (VAT_SQRT_PI * s)) * exp_term;
    float cd_term2 = (sin_alpha / s2) * (1.0 + 2.0 * s2 + epsilon * (1.0 - 2.0 * s2 * cos_2alpha)) * erf_term;
    float cd_term3 = ((1.0 - epsilon) / s) * VAT_SQRT_PI * sin2_alpha * sqrt_temp_ratio;
    float cd = cd_term1 + cd_term2 + cd_term3;

    float q = 0.5 * u_gsi_density * speed * speed;
    return (q * cl * lift_dir + q * cd * drag_dir) / max(sin_alpha, u_min_cos_delta);
}
)GLSL";

} // namespace vat::gsi_models::maxwell_glsl
