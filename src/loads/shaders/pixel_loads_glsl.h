#pragma once

namespace vat::loads::pixel_glsl {

// Pass 1: G-buffer of the per-pixel loads of the front-most surface.
//
// The GSI model is evaluated in the fragment shader, once per pixel and triangle with the
// triangle's exact normal, and the loads are written to the G-buffer. With MSAA the GPU
// copies the result into every sample the triangle covers and wins the depth test in, and
// the hardware resolve (glBlitFramebuffer) averages the samples, uncovered ones (cleared to
// 0) included. So a pixel on a crease gets the sample-weighted mean of its faces' loads,
// exactly what evaluating every sample would give: averaging the normals first and
// evaluating the model once would not, because the model is nonlinear in the normal.
// Without MSAA there is nothing to resolve; the coverage is 1 or 0.
//
// Both stages are complete gl::ShaderSource templates. The fragment shader has the placeholders
// ${num_msaa_samples} (VAT_MSAA_SAMPLES), ${write_pressure} (VAT_WRITE_PRESSURE, 0 or 1) and
// ${gsi_model}, the model's GLSL (IGSIModel::glsl_force_per_projected_area()).
// The targets and what they resolve to:
//   0: (force per projected area f, 1)      -> (mean f, coverage c)
//   1: (r x f, 1 / max(cos_d, min_cos))     -> (mean r x f, wetted-area weight)
//   2: (pressure, 0, 0, 0)                  -> (mean pressure), only if VAT_WRITE_PRESSURE
// Leeward fragments still occlude but write zeros; they are evaluated on the CPU.
inline constexpr const char* g_buffer_vertex_shader = R"GLSL(#version 430 core
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

uniform mat4 u_model;
uniform mat3 u_normal_matrix;
uniform mat4 u_view_projection;

out vec3 v_position;
flat out vec3 v_normal;

void main()
{
    vec4 body_position = u_model * vec4(a_position, 1.0);
    v_position = body_position.xyz;
    v_normal = normalize(u_normal_matrix * a_normal);
    gl_Position = u_view_projection * body_position;
}
)GLSL";

inline constexpr const char* g_buffer_fragment_shader = R"GLSL(#version 430 core
#define VAT_MSAA_SAMPLES ${num_msaa_samples}
#define VAT_WRITE_PRESSURE ${write_pressure}

uniform vec3 u_v_rel;
uniform float u_surface_temp;
uniform float u_min_cos_delta;
// vat::edge_on_threshold(|v_rel|), the leeward classification the CPU uses too.
uniform float u_edge_on_threshold;
#if VAT_MSAA_SAMPLES > 1
// Sample positions relative to the pixel centre, in pixels (GL_SAMPLE_POSITION - 0.5).
uniform vec2 u_sample_offsets[VAT_MSAA_SAMPLES];
#endif

// The GSI model: vec3 gsi_force_per_projected_area(vec3 n, vec3 v_rel, float surface_temp)
// and its own uniforms.
${gsi_model}

in vec3 v_position;
flat in vec3 v_normal;

layout (location = 0) out vec4 g_force;
layout (location = 1) out vec4 g_torque;
#if VAT_WRITE_PRESSURE
layout (location = 2) out vec4 g_pressure;
#endif

void main()
{
#if VAT_MSAA_SAMPLES > 1
    // The position at the mean of the samples the triangle covers. It is affine across the
    // triangle and r x f is linear in r, so this gives the torque of shading every sample.
    vec2 offset_sum = vec2(0.0);
    float count = 0.0;
    for (int s = 0; s < VAT_MSAA_SAMPLES; ++s)
    {
        if ((gl_SampleMaskIn[0] & (1 << s)) != 0)
        {
            offset_sum += u_sample_offsets[s];
            count += 1.0;
        }
    }
    vec2 offset = offset_sum / max(count, 1.0);
    vec3 position = v_position + dFdx(v_position) * offset.x + dFdy(v_position) * offset.y;
#else
    vec3 position = v_position; // the pixel centre, the only sample
#endif

    g_force = vec4(0.0);
    g_torque = vec4(0.0);
#if VAT_WRITE_PRESSURE
    g_pressure = vec4(0.0);
#endif
    float n_dot_v = dot(v_normal, u_v_rel);
    // Culling goes by winding, so a triangle whose normal disagrees with it can still land
    // here; leeward triangles are evaluated on the CPU.
    if (n_dot_v > u_edge_on_threshold)
    {
        float cos_d = n_dot_v / length(u_v_rel);
        vec3 f = gsi_force_per_projected_area(v_normal, u_v_rel, u_surface_temp);
        g_force = vec4(f, 1.0);
        g_torque = vec4(cross(position, f), 1.0 / max(cos_d, u_min_cos_delta));
#if VAT_WRITE_PRESSURE
        g_pressure = vec4(-dot(f, v_normal) * cos_d, 0.0, 0.0, 0.0);
#endif
    }
}
)GLSL";

// Pass 2: sum the loads over the pixels. A gl::ShaderSource template with the placeholder
// ${write_pressure} (VAT_WRITE_PRESSURE, 0 or 1).
//
// The G-buffer read here is the single-sample one of pass 1 (resolved, with MSAA). It
// already holds the loads, so this pass only scales and sums them.
//
// Each 16x16 workgroup covers a 32x32 pixel tile (every invocation sums its own 2x2 block in
// a fixed order); the workgroup then reduces its 256 sums as a binary tree in shared memory.
// All orders are fixed, so the result is bit-for-bit reproducible on a given GPU and driver.
// One partial sum per workgroup is written out and summed on the CPU in double.
inline constexpr const char* integration_shader = R"GLSL(#version 430
#define VAT_WRITE_PRESSURE ${write_pressure}

layout(local_size_x = 16, local_size_y = 16) in;

uniform float u_pixel_area;
uniform sampler2D u_g_force;
uniform sampler2D u_g_torque;
#if VAT_WRITE_PRESSURE
uniform sampler2D u_g_pressure;
#endif

// Two entries per workgroup: (force, wetted area) and (torque, windward pixels, each
// weighted by its coverage).
layout(std430, binding = 0) buffer Partials
{
    vec4 partials[];
};

#if VAT_WRITE_PRESSURE
layout(r32f, binding = 0) uniform writeonly image2D u_pressure;
#endif

shared vec4 s_force[256];
shared vec4 s_torque[256];

// The loads of pass 1, with MSAA the means over all samples, so a pixel the windward
// surface covers only partly already carries only that part of the load.
void accumulate_pixel(ivec2 pixel, inout vec4 force, inout vec4 torque)
{
    ivec2 size = textureSize(u_g_force, 0);
    if (pixel.x >= size.x || pixel.y >= size.y)
    {
        return;
    }

    float pressure = 0.0;
    vec4 force_mean = texelFetch(u_g_force, pixel, 0);
    float coverage = force_mean.w;
    if (coverage > 0.0)
    {
        vec4 torque_mean = texelFetch(u_g_torque, pixel, 0);
        force += vec4(force_mean.xyz, torque_mean.w) * u_pixel_area;
        torque += vec4(torque_mean.xyz * u_pixel_area, coverage);
#if VAT_WRITE_PRESSURE
        // The pressure is a property of the surface, so it is not scaled by the coverage.
        pressure = texelFetch(u_g_pressure, pixel, 0).x / coverage;
#endif
    }
#if VAT_WRITE_PRESSURE
    imageStore(u_pressure, pixel, vec4(pressure));
#endif
}

void main()
{
    uint local_index = gl_LocalInvocationIndex;

    vec4 force = vec4(0.0);
    vec4 torque = vec4(0.0);
    ivec2 block = ivec2(gl_WorkGroupID.xy) * 32 + ivec2(gl_LocalInvocationID.xy) * 2;
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
