#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <memory>

namespace vat::gl {

/**
 * Owns one hidden GLFW window and its OpenGL context.
 *
 * glfwInit/glfwTerminate are process-global, so they are reference counted here:
 * the library is initialised for the first context and only torn down once the
 * last one is gone. Several contexts may therefore be alive at the same time.
 */
class GlfwOpenGLContext {
private:
    struct WindowDeleter {
        void operator()(GLFWwindow* window) const { glfwDestroyWindow(window); }
    };
    std::unique_ptr<GLFWwindow, WindowDeleter> m_window;
    static bool s_glewInitialized;
    static int s_liveContexts;

public:
    GlfwOpenGLContext(int width, int height, const char* title, bool visible);
    ~GlfwOpenGLContext();

    void make_current() const;
    /// Non-owning handle; valid for as long as this context lives.
    GLFWwindow* window() const { return m_window.get(); }
};

} // namespace vat::gl
