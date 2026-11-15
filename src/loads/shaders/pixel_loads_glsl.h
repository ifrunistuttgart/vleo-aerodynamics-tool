#pragma once

namespace vat::loads::pixel_glsl {

// Pass 1: G-buffer. Writes the body-frame position and the body-frame normal of the
// front-most surface in each sample of each pixel. No GSI physics happens here, so this
// pass is the same for every model.
//
// The position is interpolated with `sample`: every sample stores the position of the
// surface at that sample's own location, so a pixel's resolved position does not depend on
// how the surface is split into triangles, and it is never extrapolated from a pixel centre
// outside the triangle. This runs the fragment shader once per sample, which costs little
// here. With one sample the sample is the pixel centre, so nothing changes.
inline constexpr const char* g_buffer_vertex_shader = R"GLSL(
#version 430 core
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

uniform mat4 u_model;
uniform mat3 u_normal_matrix;
uniform mat4 u_view_projection;

sample out vec3 v_position;
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

// Pass 2: evaluate the GSI model per pixel and reduce. Assembled as
//   #version, optional #define VAT_WRITE_PRESSURE, #define VAT_MSAA_SAMPLES n,
//   integration_header, the model's gsi_force_per_projected_area(), integration_main.
//
// With VAT_MSAA_SAMPLES > 1 the G-buffer is multisampled. The samples of a pixel are
// resolved here, in a fixed order: coverage = covered samples / VAT_MSAA_SAMPLES, the
// position is the mean over the covered samples and the normal the normalised mean. The
// GSI model is then evaluated once per pixel and scaled by the coverage. With one sample
// the code is the plain per-pixel evaluation, unchanged.
//
// Each 16x16 workgroup covers a 32x32 pixel tile: every invocation sums its own 2x2
// block in a fixed order, then the workgroup reduces its 256 sums as a binary tree in
// shared memory. Both orders are fixed, so the result is bit-for-bit reproducible.
// One partial sum per workgroup is written out and summed on the CPU in double.
inline constexpr const char* integration_header = R"GLSL(
layout(local_size_x = 16, local_size_y = 16) in;

#if VAT_MSAA_SAMPLES > 1
uniform sampler2DMS u_g_position;
uniform sampler2DMS u_g_normal;
#else
uniform sampler2D u_g_position;
uniform sampler2D u_g_normal;
#endif
uniform vec3 u_v_rel;
uniform float u_surface_temp;
uniform float u_pixel_area;
uniform float u_min_cos_delta;

// vat::edge_on_threshold(|v_rel|), computed on the CPU: pixels with dot(n, v_rel) at or
// below it are leeward and are handled per triangle on the CPU, using the same value.
uniform float u_edge_on_threshold;

// Two entries per workgroup: (force, wetted area) and (torque, windward sample count).
// The sample count is divided by VAT_MSAA_SAMPLES on the CPU; with one sample it is the
// windward pixel count.
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
    ivec2 size = textureSize(u_g_position);
    if (pixel.x >= size.x || pixel.y >= size.y)
    {
        return;
    }

    // Resolve: fixed sample order, so the result is reproducible.
    float covered = 0.0;
    vec3 position_sum = vec3(0.0);
    vec3 normal_sum = vec3(0.0);
    for (int s = 0; s < VAT_MSAA_SAMPLES; ++s)
    {
        vec4 sample_position = texelFetch(u_g_position, pixel, s);
        if (sample_position.w != 0.0)
        {
            covered += 1.0;
            position_sum += sample_position.xyz;
            normal_sum += texelFetch(u_g_normal, pixel, s).xyz;
        }
    }

    float pressure = 0.0;
    if (covered > 0.0 && dot(normal_sum, normal_sum) > 0.0)
    {
        float coverage = covered / float(VAT_MSAA_SAMPLES);
        vec3 position = position_sum / covered;
        vec3 n = normalize(normal_sum);
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
            torque += vec4(cross(position, dF), covered);
            // The pressure is a property of the surface, so it is not scaled by the coverage.
            pressure = -dot(force_per_projected_area, n) * cos_d;
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
    ivec2 block = ivec2(gl_WorkGroupID.xy) * 32 + ivec2(gl_LocalInvocationID.xy) * 2;

    vec4 force = vec4(0.0);
    vec4 torque = vec4(0.0);
    accumulate_pixel(block + ivec2(0, 0), force, torque);
    accumulate_pixel(block + ivec2(1, 0), force, torque);
    accumulate_pixel(block + ivec2(0, 1), force, torque);
    accumulate_pixel(block + ivec2(1, 1), force, torque);

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
