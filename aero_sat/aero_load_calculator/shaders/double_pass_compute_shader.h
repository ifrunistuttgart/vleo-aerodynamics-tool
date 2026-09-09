#pragma once

inline constexpr const char* compute_shader1 = R"GLSL(
#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba32f, binding = 0) uniform readonly image2D img_position;
layout(rgba32f, binding = 1) uniform readonly image2D img_pressure_vec;
layout(rgba32f, binding = 2) uniform readonly image2D img_float;

uniform float pixelArea;

struct WorkgroupResult {
    dvec4 force;  // xyz = force, w = padding (double precision)
    dvec4 torque; // xyz = torque, w = padding (double precision)
};

// Intermediate buffer to hold 1 double-precision sum per workgroup
layout(std430, binding = 3) buffer IntermediateBuffer
{
    WorkgroupResult groupResults[];
};

shared dvec3 groupForce[256];
shared dvec3 groupTorque[256];

void main()
{
    uint localID = gl_LocalInvocationIndex;
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 fb_size = imageSize(img_position);

    dvec3 pixelForce  = dvec3(0.0);
    dvec3 pixelTorque = dvec3(0.0);

    if (coord.x < fb_size.x && coord.y < fb_size.y)
    {
        // Read 32-bit float image inputs and cast immediately to double
        dvec3 position    = dvec3(imageLoad(img_position, coord).rgb);
        dvec3 pressureVec = dvec3(imageLoad(img_pressure_vec, coord).rgb);
        double cos_d      = double(imageLoad(img_float, coord).r);

        if (cos_d > 0.0)
        {
            double area = double(pixelArea) / cos_d;
            pixelForce  = pressureVec * area;
            pixelTorque = cross(position, pixelForce);
        }
    }

    groupForce[localID]  = pixelForce;
    groupTorque[localID] = pixelTorque;
    barrier();

    // Standard workgroup tree reduction in 64-bit float precision
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

    // Write double-precision workgroup sums directly to intermediate SSBO slot
    if (localID == 0u)
    {
        uint workGroupIndex = gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x;
        groupResults[workGroupIndex].force  = dvec4(groupForce[0], 0.0);
        groupResults[workGroupIndex].torque = dvec4(groupTorque[0], 0.0);
    }
}
)GLSL";

inline constexpr const char* compute_shader2 = R"GLSL(
#version 430

layout(local_size_x = 256) in;

struct WorkgroupResult {
    dvec4 force;  // double precision
    dvec4 torque; // double precision
};

layout(std430, binding = 3) buffer IntermediateBuffer
{
    WorkgroupResult groupResults[];
};

// Target SSBO matching float or double output layout
// Outputting dvec4 ensures 64-bit precision is preserved across host readback
layout(std430, binding = 4) buffer FinalBuffer
{
    dvec4 Force;  // Force.xyz, Force.w padding
    dvec4 Torque; // Torque.xyz, Torque.w padding
};

uniform uint numWorkGroupsToAggregate;

shared dvec3 sharedForce[256];
shared dvec3 sharedTorque[256];

void main()
{
    uint localID = gl_LocalInvocationIndex;
    dvec3 threadForceSum  = dvec3(0.0);
    dvec3 threadTorqueSum = dvec3(0.0);

    // Grid-stride loop: Each thread sums a slice of the intermediate double array
    for (uint i = localID; i < numWorkGroupsToAggregate; i += 256u)
    {
        threadForceSum  += groupResults[i].force.xyz;
        threadTorqueSum += groupResults[i].torque.xyz;
    }

    sharedForce[localID]  = threadForceSum;
    sharedTorque[localID] = threadTorqueSum;
    barrier();

    // Workgroup tree reduction in 64-bit float precision
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

    // Thread 0 writes the full 64-bit precision result
    if (localID == 0u)
    {
        Force  = dvec4(sharedForce[0], 0.0);
        Torque = dvec4(sharedTorque[0], 0.0);
    }
}
)GLSL";