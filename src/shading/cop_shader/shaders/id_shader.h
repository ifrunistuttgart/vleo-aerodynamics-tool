#pragma once

namespace vat::shading::cop_glsl {

// Pass 1: depth only. The fragment shader writes nothing; there is no colour attachment.
inline constexpr const char* triangle_vertex_shader = R"GLSL(
#version 430 core
layout (location = 0) in vec3 aPos;

uniform mat4 u_MVP;

void main()
{
    gl_Position = u_MVP * vec4(aPos, 1.0);
}
)GLSL";

inline constexpr const char* triangle_fragment_shader = R"GLSL(
#version 430 core

void main()
{
}
)GLSL";

// Pass 2: one point per triangle centroid.
inline constexpr const char* point_vertex_shader = R"GLSL(
#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in uint aID;
layout (location = 2) in vec3 aNormal;

flat out uint vID;

uniform mat4 u_MVP;
uniform mat4 u_MV;
// Distance (model units) the centroid is moved along its normal, towards the viewer of a
// front face. Keeps the point in front of its own triangle for the GL_LEQUAL test, since the
// point's depth and the triangle's interpolated depth are not bit-identical.
uniform float u_normal_offset;

void main()
{
    // Skip centroids of triangles whose normals face away from the camera. Only the
    // model-view transform is used to judge front/back orientation; it is exact for
    // rotations and uniform scale.
    vec3 normalView = normalize(mat3(u_MV) * aNormal);
    if (normalView.z <= 0.0) {
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0); // outside the clip volume
        vID = 0u;
        return;
    }
    vID = aID;
    gl_Position = u_MVP * vec4(aPos + aNormal * u_normal_offset, 1.0);
}
)GLSL";

// Early fragment tests make the depth test run before the shader, so the SSBO write only
// happens for points that pass GL_LEQUAL. Every writer stores the same value, so no atomic
// is needed and the result is deterministic.
inline constexpr const char* point_fragment_shader = R"GLSL(
#version 430 core
layout(early_fragment_tests) in;

flat in uint vID;

layout(std430, binding = 0) buffer VisibilityBuffer
{
    uint visible[];
};

void main()
{
    if (vID < uint(visible.length()))
    {
        visible[vID] = 1u;
    }
}
)GLSL";

} // namespace vat::shading::cop_glsl
