#pragma once

namespace vat::loads::pixel_glsl {

// Pass 1: G-buffer. Writes the body-frame position and the body-frame normal of the
// front-most surface in each sample of each pixel. No GSI physics happens here, so this
// pass is the same for every model.
//
// Without MSAA the position is interpolated at the pixel centre (`sample` changes nothing with
// one sample). With MSAA (VAT_PIXEL_RATE_POSITION) the fragment shader is
// g_buffer_fragment_shader_msaa, which stores the position at the mean of the samples the
// triangle covers, so a pixel's resolved position does not depend on how the surface is
// split into triangles.
inline constexpr const char* g_buffer_vertex_shader = R"GLSL(
#version 430 core
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

uniform mat4 u_model;
uniform mat3 u_normal_matrix;
uniform mat4 u_view_projection;

#ifdef VAT_PIXEL_RATE_POSITION
out vec3 v_position;
#else
sample out vec3 v_position;
#endif
flat out vec3 v_normal;

void main()
{
    vec4 body_position = u_model * vec4(a_position, 1.0);
    v_position = body_position.xyz;
    v_normal = normalize(u_normal_matrix * a_normal);
    gl_Position = u_view_projection * body_position;
}
)GLSL";

inline constexpr const char* g_buffer_fragment_shader = R"GLSL(
#version 430 core
sample in vec3 v_position;
flat in vec3 v_normal;

layout (location = 0) out vec4 g_position;
layout (location = 1) out vec4 g_normal;

void main()
{
    g_position = vec4(v_position, 1.0); // w = 1 marks a covered pixel; the clear value is 0
    g_normal = vec4(v_normal, 0.0);
}
)GLSL";

inline constexpr const char* g_buffer_fragment_shader_msaa = R"GLSL(
in vec3 v_position;
flat in vec3 v_normal;

layout (location = 0) out vec4 g_position;
layout (location = 1) out vec4 g_normal;

void main()
{
    vec2 offset_sum = vec2(0.0);
    float count = 0.0;
    for (int s = 0; s < VAT_MSAA_SAMPLES; ++s)
    {
        if ((gl_SampleMaskIn[0] & (1 << s)) != 0)
        {
            offset_sum += k_sample_offsets[s];
            count += 1.0;
        }
    }
    vec2 offset = offset_sum / max(count, 1.0);
    vec3 position = v_position + dFdx(v_position) * offset.x + dFdy(v_position) * offset.y;
    g_position = vec4(position, 1.0);
    g_normal = vec4(v_normal, 0.0);
}
)GLSL";

// Pass 2: evaluate the GSI model per pixel and reduce. Assembled as
//   #version, optional #define VAT_WRITE_PRESSURE, #define VAT_MSAA_SAMPLES n,
//   integration_header, the model's gsi_force_per_projected_area(), integration_main.
//
// The G-buffer read here is always single-sample. With VAT_MSAA_SAMPLES > 1 it is the
// multisampled G-buffer after a hardware resolve (glBlitFramebuffer), which averages the
// samples of each pixel, uncovered ones (cleared to 0) included:
//   w   = covered samples / N             = coverage c,
//   xyz = sum of covered positions / N    -> position = xyz / c, the mean covered position,
//   normal = sum of covered normals / N   -> n = normalize(normal), the normalised mean.
// The GSI model is evaluated once per pixel and scaled by c. With one sample the code is the
// plain per-pixel evaluation. Resolving in hardware instead of fetching every sample here is
// about 20x faster, see pixel_msaa_runtime_notes.md.
//
// Each 16x16 workgroup covers a 32x32 pixel tile without anti-aliasing (every invocation
// sums its own 2x2 block in a fixed order) and a 16x16 tile with it (one pixel per
// invocation); the workgroup then reduces its 256 sums as a binary tree in shared memory.
// All orders are fixed, so the result is bit-for-bit reproducible on a given GPU and driver.
// One partial sum per workgroup is written out and summed on the CPU in double.
inline constexpr const char* integration_header = R"GLSL(
layout(local_size_x = 16, local_size_y = 16) in;

uniform sampler2D u_g_position;
uniform sampler2D u_g_normal;
uniform vec3 u_v_rel;
uniform float u_surface_temp;
uniform float u_pixel_area;
uniform float u_min_cos_delta;

// vat::edge_on_threshold(|v_rel|), computed on the CPU: pixels with dot(n, v_rel) at or
// below it are leeward and are handled per triangle on the CPU, using the same value.
uniform float u_edge_on_threshold;

// Two entries per workgroup: (force, wetted area) and (torque, windward pixels, each
// weighted by its coverage).
layout(std430, binding = 0) buffer Partials
{
    vec4 partials[];
};

#ifdef VAT_WRITE_PRESSURE
layout(r32f, binding = 0) uniform writeonly image2D u_pressure;
#endif

shared vec4 s_force[256];
shared vec4 s_torque[256];
)GLSL";

inline constexpr const char* integration_main = R"GLSL(
#if VAT_MSAA_SAMPLES > 1
void accumulate_pixel(ivec2 pixel, inout vec4 force, inout vec4 torque)
{
    ivec2 size = textureSize(u_g_position, 0);
    if (pixel.x >= size.x || pixel.y >= size.y)
    {
        return;
    }

    float pressure = 0.0;
    vec4 position_mean = texelFetch(u_g_position, pixel, 0);
    float coverage = position_mean.w;
    if (coverage > 0.0)
    {
        vec3 normal_mean = texelFetch(u_g_normal, pixel, 0).xyz;
        if (dot(normal_mean, normal_mean) > 0.0)
        {
            vec3 position = position_mean.xyz / coverage;
            vec3 n = normalize(normal_mean);
            float n_dot_v = dot(n, u_v_rel);
            float cos_d = n_dot_v / length(u_v_rel);
            // Culling goes by winding, so a triangle whose normal disagrees with it can
            // still land here; leeward triangles are evaluated on the CPU.
            if (n_dot_v > u_edge_on_threshold)
            {
                vec3 force_per_projected_area = gsi_force_per_projected_area(n, u_v_rel, u_surface_temp);
                // A pixel the surface fills only partly carries only that part of the load.
                float covered_area = u_pixel_area * coverage;
                vec3 dF = force_per_projected_area * covered_area;
                force += vec4(dF, covered_area / max(cos_d, u_min_cos_delta));
                torque += vec4(cross(position, dF), coverage);
                // The pressure is a property of the surface, so it is not scaled by the coverage.
                pressure = -dot(force_per_projected_area, n) * cos_d;
            }
        }
    }
#ifdef VAT_WRITE_PRESSURE
    imageStore(u_pressure, pixel, vec4(pressure));
#endif
}
#else
void accumulate_pixel(ivec2 pixel, inout vec4 force, inout vec4 torque)
{
    ivec2 size = textureSize(u_g_position, 0);
    if (pixel.x >= size.x || pixel.y >= size.y)
    {
        return;
    }

    float pressure = 0.0;
    vec4 position = texelFetch(u_g_position, pixel, 0);
    if (position.w != 0.0)
    {
        vec3 n = texelFetch(u_g_normal, pixel, 0).xyz;
        float n_dot_v = dot(n, u_v_rel);
        float cos_d = n_dot_v / length(u_v_rel);
        // Culling goes by winding, so a triangle whose normal disagrees with it can
        // still land here; leeward triangles are evaluated on the CPU.
        if (n_dot_v > u_edge_on_threshold)
        {
            vec3 force_per_projected_area = gsi_force_per_projected_area(n, u_v_rel, u_surface_temp);
            vec3 dF = force_per_projected_area * u_pixel_area;
            force += vec4(dF, u_pixel_area / max(cos_d, u_min_cos_delta));
            torque += vec4(cross(position.xyz, dF), 1.0);
            pressure = -dot(force_per_projected_area, n) * cos_d;
        }
    }
#ifdef VAT_WRITE_PRESSURE
    imageStore(u_pressure, pixel, vec4(pressure));
#endif
}
#endif

void main()
{
    uint local_index = gl_LocalInvocationIndex;

    vec4 force = vec4(0.0);
    vec4 torque = vec4(0.0);
#if VAT_MSAA_SAMPLES > 1
    accumulate_pixel(ivec2(gl_WorkGroupID.xy) * 16 + ivec2(gl_LocalInvocationID.xy), force, torque);
#else
    ivec2 block = ivec2(gl_WorkGroupID.xy) * 32 + ivec2(gl_LocalInvocationID.xy) * 2;
    accumulate_pixel(block + ivec2(0, 0), force, torque);
    accumulate_pixel(block + ivec2(1, 0), force, torque);
    accumulate_pixel(block + ivec2(0, 1), force, torque);
    accumulate_pixel(block + ivec2(1, 1), force, torque);
#endif

    s_force[local_index] = force;
    s_torque[local_index] = torque;
    barrier();

    for (uint stride = 128u; stride > 0u; stride >>= 1u)
    {
        if (local_index < stride)
        {
            s_force[local_index] += s_force[local_index + stride];
            s_torque[local_index] += s_torque[local_index + stride];
        }
        barrier();
    }

    if (local_index == 0u)
    {
        uint group = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;
        partials[2u * group] = s_force[0];
        partials[2u * group + 1u] = s_torque[0];
    }
}
)GLSL";

} // namespace vat::loads::pixel_glsl
