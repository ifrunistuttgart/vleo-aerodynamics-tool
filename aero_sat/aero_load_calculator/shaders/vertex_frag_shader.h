#pragma once

// Embedded ID shader (vertex + fragment)
inline constexpr const char* ID_vertex_shader = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
uniform vec3 windDir;

flat out vec3 Normal;
out vec3 FragPos;
flat out float cos_d;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    Normal = normalMatrix * aNormal;

    vec3 nNormal = dot(Normal, Normal) > 0.0 ? normalize(Normal) : vec3(0.0);
    vec3 nWindDir = normalize(windDir);
    cos_d = dot(-nWindDir,nNormal);

    gl_Position = projection * view * worldPos;
}
)GLSL";

inline constexpr const char* ID_fragment_shader = R"GLSL(
#version 330 core
flat in vec3 Normal;
in vec3 FragPos;
flat in float cos_d;

layout (location = 0) out vec4 oFragPos;
layout (location = 1) out vec4 oNormal;
layout (location = 2) out vec4 oCos_d;

void main()
{
    oFragPos = vec4(FragPos,0.0);
    vec3 safeNormal = dot(Normal, Normal) > 0.0 ? normalize(Normal) : vec3(0.0);
    oNormal = vec4(safeNormal, 0.0);
    oCos_d = vec4(cos_d,0.0,0.0,1.0);
}
)GLSL";
