#pragma once

namespace vat::gsi_models::sentman_glsl {

// Same formula as Sentman::force_per_area(), divided by cos(delta). The thermal part
// of the momentum flux stays non-zero at grazing incidence, so the division is clamped.
inline constexpr const char* force_per_projected_area = R"GLSL(
uniform float u_gsi_density;
uniform float u_gsi_T_inf;
uniform float u_gsi_particle_mass;
uniform float u_gsi_alpha_e;
uniform int u_gsi_temperature_ratio_method;

vec3 gsi_force_per_projected_area(vec3 n, vec3 v, float Tw)
{
    float speed = length(v);

    // Most probable thermal velocity of the gas and molecular speed ratio
    float thermal_velocity = sqrt(2.0 * VAT_BOLTZMANN_CONSTANT * u_gsi_T_inf / u_gsi_particle_mass);
    float s = speed / thermal_velocity;

    float cos_delta = dot(v, n) / speed;
    float s_cos_delta = s * cos_delta;

    float inv_sqrt_pi = 1.0 / VAT_SQRT_PI;
    float exp_term = exp(-s_cos_delta * s_cos_delta);
    float erfc_term = vat_erfc(-s_cos_delta);

    float g1 = s_cos_delta * inv_sqrt_pi * exp_term + (0.5 + s_cos_delta * s_cos_delta) * erfc_term;
    float g2 = inv_sqrt_pi * exp_term + s_cos_delta * erfc_term;

    float temperature_ratio;
    if (u_gsi_temperature_ratio_method == 1)
    {
        // Exact term according to Sentman
        float enum_val = s_cos_delta * erfc_term;
        float denom = inv_sqrt_pi * exp_term + enum_val;
        temperature_ratio = u_gsi_alpha_e * (2.0 * VAT_BOLTZMANN_CONSTANT * Tw) / (u_gsi_particle_mass * speed * speed) * s * s +
            (1.0 - u_gsi_alpha_e) * (1.0 + s * s / 2.0 + 0.25 * enum_val / denom);
    }
    else if (u_gsi_temperature_ratio_method == 2)
    {
        // Hyperthermal approximation according to Tuttas
        temperature_ratio = s * s / 2.0 * (1.0 + u_gsi_alpha_e * ((4.0 * VAT_BOLTZMANN_CONSTANT * Tw) / (u_gsi_particle_mass * speed * speed) - 1.0)) +
            1.25 * (1.0 - u_gsi_alpha_e);
    }
    else
    {
        // Hyperthermal approximation according to Koppenwallner
        temperature_ratio = s * s / 2.0 * (1.0 + u_gsi_alpha_e * ((4.0 * VAT_BOLTZMANN_CONSTANT * Tw) / (u_gsi_particle_mass * speed * speed) - 1.0));
    }

    float sqrt_temperature_ratio = sqrt(temperature_ratio);
    float pressure_coeff = u_gsi_density / 2.0 * thermal_velocity * thermal_velocity;
    float sqrt_pi_half = VAT_SQRT_PI / 2.0;

    float term_1 = -(g1 + sqrt_pi_half * sqrt_temperature_ratio * g2);
    float term_2 = s * g2;

    vec3 v_rel_normalized = -v / speed;
    vec3 pressure = pressure_coeff * (term_1 * n + term_2 * (v_rel_normalized + cos_delta * n));
    return pressure / max(cos_delta, u_min_cos_delta);
}
)GLSL";

} // namespace vat::gsi_models::sentman_glsl
