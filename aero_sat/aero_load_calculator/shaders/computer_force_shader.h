#pragma once

inline constexpr const char* Compute_shader = R"GLSL(
#version 430

layout(local_size_x = 16, local_size_y = 16) in;

// Image bindings matching C++ textures
layout(rgba32f, binding = 0) uniform readonly image2D img_position;
layout(rgba32f, binding = 1) uniform readonly image2D img_normal;
layout(rgba32f, binding = 2) uniform readonly image2D img_float;

uniform float pixelArea;
uniform float density;
uniform float velocity_mag;

// SSBO using int to support atomicAdd (e.g., values in nano-Newtons)
layout(std430, binding = 3) buffer LoadBuffer
{
    ivec3 Force;  // Force.x, Force.y, Force.z
    ivec3 Torque; // Torque.x, Torque.y, Torque.z
};

// 1. Declare shared memory variables for workgroup-level accumulation
shared ivec3 groupForce[256];
shared ivec3 groupTorque[256];

void main()
{
    uint localID = gl_LocalInvocationIndex; // Flat index [0..255]
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 fb_size = imageSize(img_position);

    ivec3 pixelForce  = ivec3(0);
    ivec3 pixelTorque = ivec3(0);

    // Compute pixel forces if thread is inside valid texture bounds
    if (coord.x < fb_size.x && coord.y < fb_size.y)
    {
        vec3 position = imageLoad(img_position, coord).rgb;
        vec3 normal   = imageLoad(img_normal, coord).rgb;
        float cos_d   = imageLoad(img_float, coord).r;

        if (cos_d > 0.0)
        {
            float cp = 2.0 * cos_d * cos_d;
            float area = pixelArea / cos_d;
            pixelForce  = ivec3(-0.5 * density * velocity_mag * velocity_mag * cp * area * 1.0e9 * normal);
            pixelTorque = ivec3(cross(position, vec3(pixelForce)));
        }
    }

    // 2. Write individual thread contributions to shared memory
    groupForce[localID]  = pixelForce;
    groupTorque[localID] = pixelTorque;

    // Barrier ensures all threads in the workgroup have written to shared memory
    barrier();

    // 3. Parallel tree reduction within the workgroup (256 threads -> 1 total)
    for (uint stride = 128u; stride > 0u; stride >>= 1u)
    {
        if (localID < stride)
        {
            groupForce[localID]  += groupForce[localID + stride];
            groupTorque[localID] += groupTorque[localID + stride];
        }
        memoryBarrierShared();
        barrier();
    }

    // 4. Thread 0 writes the single workgroup total to global SSBO memory via atomicAdd
    if (localID == 0u)
    {
        atomicAdd(Force.x, groupForce[0].x);
        atomicAdd(Force.y, groupForce[0].y);
        atomicAdd(Force.z, groupForce[0].z);

        atomicAdd(Torque.x, groupTorque[0].x);
        atomicAdd(Torque.y, groupTorque[0].y);
        atomicAdd(Torque.z, groupTorque[0].z);
    }
}
)GLSL";