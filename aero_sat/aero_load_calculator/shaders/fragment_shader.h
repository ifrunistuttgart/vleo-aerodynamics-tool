#pragma once

inline constexpr const char* gsi_fragment_shader = R"GLSL(
#version 330 core
flat in vec3 pressureVec;
in vec3 FragPos;
flat in float cos_d;

layout (location = 0) out vec4 oFragPos;
layout (location = 1) out vec4 oPressureVec;
layout (location = 2) out vec4 oCos_d;

void main()
{
    oFragPos = vec4(FragPos,0.0);
    oPressureVec = vec4(pressureVec, 0.0);
    oCos_d = vec4(cos_d,0.0,0.0,1.0);
}
)GLSL";
