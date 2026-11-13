#pragma once

namespace vat::shading::binary_glsl {

// One program serves both passes, so pass 1 (depth) and pass 2 (mark) are guaranteed to
// run the same vertex code; `invariant` additionally pins gl_Position bit-for-bit, which
// the GL_LEQUAL test in pass 2 relies on.
inline constexpr const char* vertex_shader = R"GLSL(
#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in uint aID;

flat out uint vID;
invariant gl_Position;

uniform mat4 u_MVP;

void main()
{
    vID = aID;
    gl_Position = u_MVP * vec4(aPos, 1.0);
}
)GLSL";

// Early fragment tests make the (per-sample) depth test run before the shader, so the
// SSBO write happens only for fragments with at least one sample that passed it. With
// MSAA the shader runs once per pixel and triangle if any of its samples is covered and
// passes.
inline constexpr const char* fragment_shader = R"GLSL(
#version 430 core
layout(early_fragment_tests) in;

flat in uint vID;

layout(std430, binding = 0) buffer VisibilityBuffer
{
    uint visible[];
};

uniform bool u_mark;

void main()
{
    // Every writer stores the same value, so no atomic is needed and the result is
    // deterministic.
    if (u_mark && vID < uint(visible.length()))
    {
        visible[vID] = 1u;
    }
}
)GLSL";

} // namespace vat::shading::binary_glsl
