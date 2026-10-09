#pragma once

namespace vat::shading::cop_glsl {

// Embedded ID shader (vertex + fragment)
inline constexpr const char* ID_vertex_shader = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in uint aColor;

flat out uint vColor;

uniform mat4 u_MVP;

void main()
{
    vColor = aColor;
    gl_Position = u_MVP * vec4(aPos, 1.0);
}
)GLSL";

inline constexpr const char* ID_point_shader = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in uint aColor;
layout (location = 2) in vec3 aNormal;

flat out uint vColor;

uniform mat4 u_MVP;
uniform mat4 u_MV;

void main()
{
    vec3 normalView = normalize(mat3(u_MV) * aNormal);
    // Skip centroids for triangles whose normals face away from the camera.
    // We only use the model-view transform to judge front/back orientation.
    if (normalView.z <= 0.0) {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        return;
    }

    vColor = aColor;
    // Offset the point along its normal to avoid z-fighting with the triangle surface
    // The offset is a small fraction of the point's depth to ensure it's always in front
    vec3 offsetPos = aPos + aNormal * 0.005;
    vec4 pos = u_MVP * vec4(offsetPos, 1.0);
    gl_Position = pos;
}
)GLSL";

inline constexpr const char* ID_fragment_shader = R"GLSL(
#version 330 core
flat in uint vColor;
out uvec4 FragColor;

void main()
{
    FragColor = uvec4(vColor, 0u, 0u, 255u);
}
)GLSL";

} // namespace vat::shading::cop_glsl
