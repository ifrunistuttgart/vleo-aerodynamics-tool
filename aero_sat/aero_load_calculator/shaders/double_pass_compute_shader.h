#pragma once

inline  constexpr const char* compute_shader1 = R"GLSL(
#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform readonly image2D img_position;
layout(rgba32f, binding = 1) uniform readonly image2D img_pressure_vec;
layout(rgba32f, binding = 2) uniform readonly image2D img_float;

uniform float pixelArea;

struct WorkgroupResult {
    vec4 force;  // xyz = force, w = padding
    vec4 torque; // xyz = torque, w = padding
};

// Intermediate buffer to hold 1 sum per workgroup
layout(std430, binding = 3) buffer IntermediateBuffer
{
    WorkgroupResult groupResults[];
};

shared vec3 groupForce[256];
shared vec3 groupTorque[256];

void main()
{
    uint localID = gl_LocalInvocationIndex;
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 fb_size = imageSize(img_position);

    vec3 pixelForce  = vec3(0.0);
    vec3 pixelTorque = vec3(0.0);

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

    groupForce[localID]  = pixelForce;
    groupTorque[localID] = pixelTorque;
    barrier();

    // Standard workgroup tree reduction
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

    // Write workgroup sums directly to dedicated slot (No global atomics)
    if (localID == 0u)
    {
        uint workGroupIndex = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;
        groupResults[workGroupIndex].force  = vec4(groupForce[0], 0.0);
        groupResults[workGroupIndex].torque = vec4(groupTorque[0], 0.0);
    }
}
)GLSL";

inline constexpr const char* compute_shader2 = R"GLSL(
#version 430

layout(local_size_x = 256) in;

struct WorkgroupResult {
    vec4 force;
    vec4 torque;
};

layout(std430, binding = 3) buffer IntermediateBuffer
{
    WorkgroupResult groupResults[];
};

// Target SSBO matching ForceTorqueData struct layout
layout(std430, binding = 4) buffer FinalBuffer
{
    vec4 Force;  // Force.xyz, Force.w padding
    vec4 Torque; // Torque.xyz, Torque.w padding
};

uniform uint numWorkGroupsToAggregate;
uniform float scaleFactor;

shared vec3 sharedForce[256];
shared vec3 sharedTorque[256];

void main()
{
    uint localID = gl_LocalInvocationIndex;
    vec3 threadForceSum  = vec3(0.0);
    vec3 threadTorqueSum = vec3(0.0);

    // Grid-stride loop: Each thread sums a slice of the intermediate array
    for (uint i = localID; i < numWorkGroupsToAggregate; i += 256u)
    {
        threadForceSum  += groupResults[i].force.xyz;
        threadTorqueSum += groupResults[i].torque.xyz;
    }

    sharedForce[localID]  = threadForceSum;
    sharedTorque[localID] = threadTorqueSum;
    barrier();

    // Workgroup tree reduction
    for (uint stride = 128u; stride > 0u; stride >>= 1u)
    {
        if (localID < stride)
        {
            sharedForce[localID]  += sharedForce[localID + stride];
            sharedTorque[localID] += sharedTorque[localID + stride];
        }
        memoryBarrierShared();
        barrier();
    }

    // Thread 0 writes the precise total sum converted to scaled integers
    if (localID == 0u)
    {
        Force  = vec4(sharedForce[0], 0.0);
        Torque = vec4(sharedTorque[0], 0.0);
    }
}
)GLSL";