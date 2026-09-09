#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

/**
 * Owns one hidden GLFW window and its OpenGL context.
 *
 * glfwInit/glfwTerminate are process-global, so they are reference counted here:
 * the library is initialised for the first context and only torn down once the
 * last one is gone. Several contexts may therefore be alive at the same time.
 */
class GlfwOpenGLContext {
private:
    GLFWwindow* m_window = nullptr;
    static bool s_glewInitialized;
    static int s_liveContexts;

public:
    GlfwOpenGLContext(int width, int height, const char* title, bool visible);
    ~GlfwOpenGLContext();

    void make_current() const;
    GLFWwindow* window() const { return m_window; }
};