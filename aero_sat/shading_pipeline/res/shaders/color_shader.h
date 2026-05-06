#pragma once

inline constexpr const char* Color_vertex_shader = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in uint aColor;

flat out vec4 vColor;

uniform mat4 u_MVP;

void main()
{
    // Extrahiere RGBA-Komponenten aus dem uint-Wert
    float r = float((aColor >> 16) & 0xFFu) / 255.0f;
    float g = float((aColor >> 8) & 0xFFu) / 255.0f;
    float b = float((aColor) & 0xFFu) / 255.0f;
    float a = 1.0;
    
    vColor = vec4(r, g, b, a);
    gl_Position = u_MVP * vec4(aPos, 1.0);
}
)GLSL";

inline constexpr const char* Color_fragment_shader = R"GLSL(
#version 330 core
flat in vec4 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vColor; // Orange color

}
)GLSL";
