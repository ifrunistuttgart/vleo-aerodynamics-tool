#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

class GlfwOpenGLContext {
private:
    GLFWwindow* m_window = nullptr;
    static bool s_glew_initialized;

public:
    GlfwOpenGLContext(int width, int height, const char* title, bool visible);
    ~GlfwOpenGLContext();

    void make_current() const;
    GLFWwindow* window() const { return m_window; }
};