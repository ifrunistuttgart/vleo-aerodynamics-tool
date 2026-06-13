#include "glfw_opengl_context.h"
#include <stdexcept>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

bool GlfwOpenGLContext::s_glewInitialized = false;

GlfwOpenGLContext::GlfwOpenGLContext(int width, int height, const char* title, bool visible) {
    if (!glfwInit()) {
        SPDLOG_ERROR("glfwInit failed");
        throw std::runtime_error("glfwInit failed");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (m_window == nullptr) {
        SPDLOG_ERROR("glfwCreateWindow failed (size={}x{}, title='{}', visible={})", width, height, title, visible);
        glfwTerminate();
        throw std::runtime_error("glfwCreateWindow failed");
    }

    glfwMakeContextCurrent(m_window);

    if (!s_glewInitialized) {
        const GLenum glew_result = glewInit();
        if (glew_result != GLEW_OK) {
            SPDLOG_ERROR("glewInit failed (code={})", static_cast<unsigned int>(glew_result));
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
