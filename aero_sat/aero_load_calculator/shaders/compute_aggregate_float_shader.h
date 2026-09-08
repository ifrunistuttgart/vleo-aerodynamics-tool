#pragma once

inline constexpr const char* Compute_shader_float = R"GLSL(
#version 430

layout(local_size_x = 32, local_size_y = 32) in;

// Image bindings matching C++ textures
layout(rgba32f, binding = 0) uniform readonly image2D img_position;
layout(rgba32f, binding = 1) uniform readonly image2D img_pressure_vec;
layout(rgba32f, binding = 2) uniform readonly image2D img_float;

uniform float pixelArea;

// Float SSBO payload typed as uint to support GLSL atomic operations
layout(std430, binding = 3) buffer LoadBuffer
{
    uvec4 Force;  // Force.xyz, Force.w is padding
    uvec4 Torque; // Torque.xyz, Torque.w is padding
};

// Shared memory for workgroup reduction + compensation tracking
shared vec3 groupForce[256];
shared vec3 groupTorque[256];
shared vec3 compForce[256];
shared vec3 compTorque[256];

// Helper: Kahan summation step for vec3
// Adds 'val' to 'sum' while tracking lost precision in 'c'
void kahanAdd(inout vec3 sum, inout vec3 c, vec3 val)
{
    vec3 y = val - c;         // Subtract lost precision from incoming value
    vec3 t = sum + y;         // Add to running sum (low bits may be lost here)
    c = (t - sum) - y;        // Recover low bits lost during (sum + y)
    sum = t;                  // Update running sum
}

void main()
{
    uint localID = gl_LocalInvocationIndex; // Flat index [0..255]
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 fb_size = imageSize(img_position);

    vec3 pixelForce  = vec3(0.0);
    vec3 pixelTorque = vec3(0.0);

    // 1. Compute pixel forces inside valid bounds
    if (coord.x < fb_size.x && coord.y < fb_size.y)
    {
        vec3 position    = imageLoad(img_position, coord).rgb;
        vec3 pressureVec = imageLoad(img_pressure_vec, coord).rgb;
        float cos_d      = imageLoad(img_float, coord).r;

        if (cos_d > 0.0)
        {
            float area  = pixelArea / cos_d;
            pixelForce  = pressureVec * area;
            pixelTorque = cross(position, pixelForce);
        }
    }

    // 2. Write initial thread contributions and zero out compensations
    groupForce[localID]  = pixelForce;
    groupTorque[localID] = pixelTorque;
    compForce[localID]   = vec3(0.0);
    compTorque[localID]  = vec3(0.0);
    barrier();

    // 3. Compensated Tree Reduction within the workgroup
    for (uint stride = 128u; stride > 0u; stride >>= 1u)
    {
        if (localID < stride)
        {
            // Combine both the value and accumulated error from the paired thread
            kahanAdd(groupForce[localID],  compForce[localID],  groupForce[localID + stride]  - compForce[localID + stride]);
            kahanAdd(groupTorque[localID], compTorque[localID], groupTorque[localID + stride] - compTorque[localID + stride]);
        }
        memoryBarrierShared();
        barrier();
    }

    // 4. Thread 0 writes the precise workgroup sum directly to global SSBO
    if (localID == 0u)
    {
        // Net sum includes recovered lower-order bits
        vec3 totalForce  = groupForce[0]  - compForce[0];
        vec3 totalTorque = groupTorque[0] - compTorque[0];

        // Global Atomic CAS loop for Force
        if (any(notEqual(totalForce, vec3(0.0))))
        {
            for (int i = 0; i < 3; ++i)
            {
                if (totalForce[i] != 0.0)
                {
                    uint expected;
                    uint current = Force[i];
                    do {
                        expected = current;
                        float sum = uintBitsToFloat(expected) + totalForce[i];
                        current = atomicCompSwap(Force[i], expected, floatBitsToUint(sum));
                    } while (current != expected);
                }
            }
        }

        // Global Atomic CAS loop for Torque
        if (any(notEqual(totalTorque, vec3(0.0))))
        {
            for (int i = 0; i < 3; ++i)
            {
                if (totalTorque[i] != 0.0)
                {
                    uint expected;
                    uint current = Torque[i];
                    do {
                        expected = current;
                        float sum = uintBitsToFloat(expected) + totalTorque[i];
                        current = atomicCompSwap(Torque[i], expected, floatBitsToUint(sum));
                    } while (current != expected);
                }
            }
        }
    }
}
)GLSL";