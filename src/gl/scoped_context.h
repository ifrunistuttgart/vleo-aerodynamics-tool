#pragma once
#include "glfw_opengl_context.h"

namespace vat::gl {

/**
 * Makes a context current for the lifetime of the guard, then restores whichever
 * context was current before.
 *
 * Every GPU object in this toolbox owns its own hidden context, and a caller such as
 * the MATLAB gateway alternates between them. Restoring the previous context keeps one
 * object's call from silently changing the context another one draws into.
 */
class ScopedCurrentContext {
public:
    explicit ScopedCurrentContext(const GlfwOpenGLContext& context)
        : m_previous(glfwGetCurrentContext()) {
        context.make_current();
    }

    ~ScopedCurrentContext() {
        glfwMakeContextCurrent(m_previous);
    }

    ScopedCurrentContext(const ScopedCurrentContext&) = delete;
    ScopedCurrentContext& operator=(const ScopedCurrentContext&) = delete;

private:
    GLFWwindow* m_previous;
};

} // namespace vat::gl
