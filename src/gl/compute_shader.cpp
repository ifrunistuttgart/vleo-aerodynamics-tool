#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <string>

#include "gl_helpers.h"
#include "compute_shader.h"

namespace vat::gl {

// Constructor: create compute shader directly from provided source string
ComputeShader::ComputeShader(const std::string& compute_source) : m_file_path(""), m_compute_shader_id(0) {
    if (compute_source.empty()) {
        SPDLOG_ERROR("Computeshader source is empty");
        return;
    }
    m_compute_shader_id = create_shader(compute_source);
}

ComputeShader::ComputeShader(const ShaderSource& compute_source) : m_file_path(""), m_compute_shader_id(0) {
    m_compute_shader_id = create_shader(compute_source.glsl());
}

ComputeShader::~ComputeShader() { GLCall(glDeleteProgram(m_compute_shader_id)); }

void ComputeShader::bind() const { GLCall(glUseProgram(m_compute_shader_id)); }

void ComputeShader::unbind() const { GLCall(glUseProgram(0)); }

void ComputeShader::dispatch(unsigned int groups_x, unsigned int groups_y, unsigned int groups_z) const {
    GLCall(glDispatchCompute(groups_x, groups_y, groups_z));
}

void ComputeShader::set_uniform_1i(const std::string& name, int value) {
    GLCall(glUniform1i(get_uniform_location(name), value));
}

void ComputeShader::set_uniform_1f(const std::string& name, float value) {
    GLCall(glUniform1f(get_uniform_location(name), value));
}

void ComputeShader::set_uniform_3f(const std::string& name, const glm::vec3& value) {
    GLCall(glUniform3f(get_uniform_location(name), value.x, value.y, value.z));
}

void ComputeShader::set_uniform_4f(const std::string& name, float v0, float v1, float v2, float v3) {
    GLCall(glUniform4f(get_uniform_location(name), v0, v1, v2, v3));
}

void ComputeShader::set_uniform_mat4f(const std::string& name, const glm::mat4& matrix) {
    GLCall(glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, &matrix[0][0]));
}

int ComputeShader::get_uniform_location(const std::string& name) {
    if (m_uniform_location_cache.find(name) != m_uniform_location_cache.end())
        return m_uniform_location_cache[name];

    GLCall(int location = glGetUniformLocation(m_compute_shader_id, name.c_str()));
    if (location == -1)
        SPDLOG_WARN("Uniform {} doesn't exist!", name);
    m_uniform_location_cache[name] = location;
    return location;
}

unsigned int ComputeShader::create_shader(const std::string& source) {
    unsigned int program = glCreateProgram();
    unsigned int cs = compile_shader(GL_COMPUTE_SHADER, source);

    if (cs == 0) {
        SPDLOG_ERROR("Compute shader compilation failed, aborting shader program creation.");
        return 0;
    }

    glAttachShader(program, cs);
    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(cs);

    return program;
}

unsigned int ComputeShader::compile_shader(unsigned int type, const std::string& source) {
    if (source.empty()) {
        SPDLOG_ERROR("Compute shader source is empty, cannot compile shader.");
        return 0;
    }

    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        std::string message(length, '\0');
        glGetShaderInfoLog(id, length, &length, &message[0]);
        SPDLOG_ERROR("Failed to compile compute shader!\nInfoLog:\n{}", message);

        const GLubyte* glVersion = glGetString(GL_VERSION);
        const GLubyte* glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
        SPDLOG_ERROR("OpenGL: {}\nGLSL: {}", glVersion ? reinterpret_cast<const char*>(glVersion) : "unknown",
                     glsl_version ? reinterpret_cast<const char*>(glsl_version) : "unknown");

        SPDLOG_ERROR("Compute shader source:\n{}", ShaderSource::numbered(source));

        glDeleteShader(id);
        return 0;
    }
    return id;
}

} // namespace vat::gl
