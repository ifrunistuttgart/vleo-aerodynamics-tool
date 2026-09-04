#pragma once

inline constexpr const char* Compute_shader = R"GLSL(
#version 430

layout(local_size_x = 16, local_size_y = 16) in;

// Image bindings matching C++ textures
layout(rgba32f, binding = 0) uniform readonly image2D img_position;
layout(rgba32f, binding = 1) uniform readonly image2D img_pressure_vec;
layout(rgba32f, binding = 2) uniform readonly image2D img_float;

uniform float pixelArea;

// SSBO using int to support atomicAdd (values are scaled to pico-Newtons)
layout(std430, binding = 3) buffer LoadBuffer
{
    ivec4 Force;  // Force.xyz, Force.w is padding
    ivec4 Torque; // Torque.xyz, Torque.w is padding
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
        vec3 pressureVec   = imageLoad(img_pressure_vec, coord).rgb;
        float cos_d   = imageLoad(img_float, coord).r;

        if (cos_d > 0.0)
        {
            float area = pixelArea / cos_d;
            pixelForce  = ivec3(pressureVec * area * 1.0e12 );
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

    // 4. Thread 0 writes the single workgroup total to global SSBO memory via atomicAdd.
    //    Skip global writes entirely when both force and torque sums are zero.
    if (localID == 0u)
    {
        ivec3 totalForce = groupForce[0];
        ivec3 totalTorque = groupTorque[0];

        if (any(notEqual(totalForce, ivec3(0))) || any(notEqual(totalTorque, ivec3(0))))
        {
            atomicAdd(Force.x, totalForce.x);
            atomicAdd(Force.y, totalForce.y);
            atomicAdd(Force.z, totalForce.z);

            atomicAdd(Torque.x, totalTorque.x);
            atomicAdd(Torque.y, totalTorque.y);
            atomicAdd(Torque.z, totalTorque.z);
        }
    }
}
)GLSL";