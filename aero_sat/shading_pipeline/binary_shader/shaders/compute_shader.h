#pragma once

inline constexpr const char* Compute_shader = R"GLSL(
#version 430

layout(local_size_x = 16, local_size_y = 16) in;

layout(r32ui, binding = 0) uniform uimage2D framebuffer_texture;

layout(std430, binding = 1) buffer HistogramBuffer 
{
    uint histogram[];
};

void main() 
{
    ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
    ivec2 fb_size = imageSize(framebuffer_texture);
    if (coord.x >= fb_size.x || coord.y >= fb_size.y) return;
    uint triangle_id = imageLoad(framebuffer_texture, coord).r;
    if (triangle_id > 0 && triangle_id < histogram.length()) 
    {
        atomicAdd(histogram[triangle_id], 1u);
    }
}
)GLSL";
