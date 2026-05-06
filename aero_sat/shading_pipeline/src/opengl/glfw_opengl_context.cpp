#include "src/opengl/glfw_opengl_context.h"
#include <stdexcept>

bool GlfwOpenGLContext::s_glewInitialized = false;

GlfwOpenGLContext::GlfwOpenGLContext(int width, int height, const char* title, bool visible) {
    if (!glfwInit()) {
        throw std::runtime_error("glfwInit failed");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (m_window == nullptr) {
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow failed");
    }

    glfwMakeContextCurrent(m_window);

    if (!s_glewInitialized) {
        if (glewInit() != GLEW_OK) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
            glfwTerminate();
            throw std::runtime_error("glewInit failed");
        }
        s_glewInitialized = true;
    }
}

GlfwOpenGLContext::~GlfwOpenGLContext() {
    if (m_window != nullptr) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

void GlfwOpenGLContext::make_current() const {
    glfwMakeContextCurrent(m_window);
}