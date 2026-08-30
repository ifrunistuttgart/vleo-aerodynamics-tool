#pragma once

inline constexpr const char* Compute_shader = R"GLSL(
#version 430

layout(local_size_x = 16, local_size_y = 16) in;

// Image bindings matching C++ textures
// Note: RGB formats must use rgba layout qualifiers in GLSL
layout(rgba32f, binding = 0) uniform readonly image2D img_position; // GL_RGB32F (or GL_RGBA32F)
layout(rgba32f, binding = 1) uniform readonly image2D img_normal;   // GL_RGBA32F
layout(rgba32f, binding = 2) uniform readonly image2D img_float;    // GL_R32F

uniform float pixelArea;
uniform float density;
uniform float velocity_mag;
// SSBO using int or uint to support atomicAdd (e.g., values in nano-Newtons)
layout(std430, binding = 3) buffer LoadBuffer
{
    ivec3 Force;  // Force.x, Force.y, Force.z
    ivec3 Torque; // Torque.x, Torque.y, Torque.z
};

void main()
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 fb_size = imageSize(img_position);
    
    if (coord.x >= fb_size.x || coord.y >= fb_size.y) return;

    // Read from your 3 textures
    vec3 position = imageLoad(img_position, coord).rgb;
    vec3 normal   = imageLoad(img_normal, coord).rgb;
    float cos_d   = imageLoad(img_float, coord).r;

    // Compute pixel forces (convert to integer scale for atomicAdd, e.g., scaled to nano-Newtons)
    //dummy values
    float cp = 2*cos_d*cos_d;
    float area = pixelArea/cos_d;
    ivec3 pixelForce  = ivec3(-0.5*density*velocity_mag*velocity_mag*cp*area*1.0e9*normal);
    ivec3 pixelTorque = ivec3(cross(position, pixelForce));

    // Perform atomic additions using correct 0-based indexing [0, 1, 2]
    atomicAdd(Force[0], pixelForce[0]);
    atomicAdd(Force[1], pixelForce[1]);
    atomicAdd(Force[2], pixelForce[2]);

    atomicAdd(Torque[0], pixelTorque[0]);
    atomicAdd(Torque[1], pixelTorque[1]);
    atomicAdd(Torque[2], pixelTorque[2]);
}
)GLSL";

