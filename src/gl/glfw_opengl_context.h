#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <memory>

namespace vat::gl {

/**
 * @brief Owns one hidden GLFW window and its OpenGL context.
 *
 * glfwInit/glfwTerminate are process-global, so they are reference counted here:
 * the library is initialised for the first context and only torn down once the
 * last one is gone. Several contexts may therefore be alive at the same time. The
 * reference count is a plain static, so contexts must be created and destroyed from one
 * thread.
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
    /**
     * @brief Creates the window and an OpenGL 4.3 core context, and makes the context current.
     *
     * @param width Window width in pixels.
     * @param height Window height in pixels.
     * @param title Window title.
     * @param visible Whether the window is shown.
     * @throws std::runtime_error if glfwInit, glfwCreateWindow or glewInit fails.
     */
    GlfwOpenGLContext(int width, int height, const char* title, bool visible);
    ~GlfwOpenGLContext();

    /** @brief Makes this context current on the calling thread. */
    void make_current() const;
    /** @brief Non-owning handle; valid for as long as this context lives. */
    GLFWwindow* window() const { return m_window.get(); }
};

} // namespace vat::gl
