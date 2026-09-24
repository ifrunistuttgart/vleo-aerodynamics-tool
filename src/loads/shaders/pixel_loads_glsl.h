#pragma once

namespace vat::loads::pixel_glsl {

// Pass 1: G-buffer. Writes the body-frame position and the body-frame normal of the
// front-most surface in each pixel. No GSI physics happens here, so this pass is the
// same for every model.
inline constexpr const char* g_buffer_vertex_shader = R"GLSL(
#version 430 core
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

inline constexpr const char* g_buffer_fragment_shader = R"GLSL(
#version 430 core
in vec3 v_position;
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
//   #version, optional #define VAT_WRITE_PRESSURE, integration_header,
//   the model's gsi_force_per_projected_area(), integration_main.
//
// Each 16x16 workgroup covers a 32x32 pixel tile: every invocation sums its own 2x2
// block in a fixed order, then the workgroup reduces its 256 sums as a binary tree in
// shared memory. Both orders are fixed, so the result is bit-for-bit reproducible.
// One partial sum per workgroup is written out and summed on the CPU in double.
inline constexpr const char* integration_header = R"GLSL(
layout(local_size_x = 16, local_size_y = 16) in;

uniform sampler2D u_g_position;
uniform sampler2D u_g_normal;
uniform vec3 u_v_rel;
uniform float u_surface_temp;
uniform float u_pixel_area;
uniform float u_min_cos_delta;

// Two entries per workgroup: (force, wetted area) and (torque, windward pixel count).
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
        float cos_d = dot(n, u_v_rel) / length(u_v_rel);
        // Leeward pixels only occlude; their triangles are evaluated on the CPU.
        if (cos_d > 0.0)
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
