#include "glfw_opengl_context.h"
#include <stdexcept>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <format>
bool GlfwOpenGLContext::s_glewInitialized = false;
int GlfwOpenGLContext::s_liveContexts = 0;

GlfwOpenGLContext::GlfwOpenGLContext(int width, int height, const char* title, bool visible) {
    if (s_liveContexts == 0 && !glfwInit()) {
        SPDLOG_ERROR("glfwInit failed");
        throw std::runtime_error("glfwInit failed");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (m_window == nullptr) {
        const char* description;
        int errorCode = glfwGetError(&description);
        SPDLOG_ERROR("glfwCreateWindow failed! Error Code: {} | Description: {}", errorCode, description ? description : "Unknown");
        SPDLOG_ERROR("glfwCreateWindow failed (size={}x{}, title='{}', visible={})", width, height, title, visible);
        if (s_liveContexts == 0) {
            glfwTerminate();
        }
        throw std::runtime_error(std::format("glfwCreateWindow failed (size={}x{}, title='{}', visible={}): {}", width, height, title, visible, std::string(description ? description : "Unknown")));
    }

    glfwMakeContextCurrent(m_window);

    if (!s_glewInitialized) {
        const GLenum glew_result = glewInit();
        if (glew_result != GLEW_OK) {
            SPDLOG_ERROR("glewInit failed (code={})", static_cast<unsigned int>(glew_result));
            glfwDestroyWindow(m_window);
            m_window = nullptr;
            if (s_liveContexts == 0) {
                glfwTerminate();
            }
            throw std::runtime_error("glewInit failed");
        }
        s_glewInitialized = true;
    }

    ++s_liveContexts;
}

GlfwOpenGLContext::~GlfwOpenGLContext() {
    if (m_window == nullptr) {
        return; // construction failed; this context never counted
    }
    glfwDestroyWindow(m_window);
    m_window = nullptr;

    if (--s_liveContexts == 0) {
        glfwTerminate();
        s_glewInitialized = false; // the next glfwInit needs GLEW resolved again
    }
}

void GlfwOpenGLContext::make_current() const {
    glfwMakeContextCurrent(m_window);
}
