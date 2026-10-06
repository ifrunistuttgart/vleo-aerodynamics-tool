#pragma once

namespace vat::gsi_models::common_glsl {

// Helpers shared by the models' GLSL. Each model's source starts with this block; a
// shader only ever contains one model, so the names cannot collide.
inline constexpr const char* helpers = R"GLSL(
const float VAT_BOLTZMANN_CONSTANT = 1.380649e-23; // J/K, as BOLTZMANN_CONSTANT__J_PER_K in core.h
const float VAT_SQRT_PI = 1.77245385;

// GLSL has no erfc. Chebyshev fit from Numerical Recipes (erfcc), fractional error
// below 1.2e-7 for every x, so erfc stays accurate where it is tiny as well.
float vat_erfc(float x)
{
    float z = abs(x);
    float t = 1.0 / (1.0 + 0.5 * z);
    float r = t * exp(-z * z - 1.26551223 + t * (1.00002368 + t * (0.37409196 + t * (0.09678418 +
        t * (-0.18628806 + t * (0.27886807 + t * (-1.13520398 + t * (1.48851587 +
        t * (-0.82215223 + t * 0.17087277)))))))));
    return x >= 0.0 ? r : 2.0 - r;
}

float vat_erf(float x)
{
    return 1.0 - vat_erfc(x);
}

// Direction of lift: perpendicular to the velocity, in the plane of normal and velocity.
// Same as the C++ models: -normalize(cross(cross(v, n), v)), zero at normal incidence.
vec3 vat_lift_direction(vec3 n, vec3 v)
{
    vec3 flow_normal_cross = cross(v, n);
    if (length(flow_normal_cross) > 1e-10)
    {
        return -normalize(cross(flow_normal_cross, v));
    }
    return vec3(0.0);
}
)GLSL";

} // namespace vat::gsi_models::common_glsl
