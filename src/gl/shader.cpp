#include "shader.h"

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <string>
#include <vector>

#include "gl_helpers.h"

namespace vat::gl {

Shader::Shader(const std::string& vertex_source, const std::string& fragment_source) : m_file_path(""), m_shader_id(0) {
    m_shader_id = create_shader(vertex_source, fragment_source);
}

Shader::Shader(const ShaderSource& vertex_source, const ShaderSource& fragment_source)
    : m_file_path(""), m_shader_id(0) {
    m_shader_id = create_shader(vertex_source.glsl(), fragment_source.glsl());
}

Shader::~Shader() { GLCall(glDeleteProgram(m_shader_id)); }

void Shader::bind() const { GLCall(glUseProgram(m_shader_id)); }

void Shader::unbind() const { GLCall(glUseProgram(0)); }

void Shader::set_uniform_1i(const std::string& name, int value) {
    GLCall(glUniform1i(get_uniform_location(name), value));
}

void Shader::set_uniform_1f(const std::string& name, float value) {
    GLCall(glUniform1f(get_uniform_location(name), value));
}

void Shader::set_uniform_3f(const std::string& name, const glm::vec3& value) {
    GLCall(glUniform3f(get_uniform_location(name), value.x, value.y, value.z));
}

void Shader::set_uniform_2fv(const std::string& name, const std::vector<glm::vec2>& values) {
    GLCall(glUniform2fv(get_uniform_location(name), static_cast<GLsizei>(values.size()), &values[0][0]));
}

void Shader::set_uniform_4f(const std::string& name, float v0, float v1, float v2, float v3) {
    GLCall(glUniform4f(get_uniform_location(name), v0, v1, v2, v3));
}

void Shader::set_uniform_mat3f(const std::string& name, const glm::mat3& matrix) {
    GLCall(glUniformMatrix3fv(get_uniform_location(name), 1, GL_FALSE, &matrix[0][0]));
}

void Shader::set_uniform_mat4f(const std::string& name, const glm::mat4& matrix) {
    GLCall(glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, &matrix[0][0]));
}

int Shader::get_uniform_location(const std::string& name) {
    if (m_uniform_location_cache.find(name) != m_uniform_location_cache.end())
        return m_uniform_location_cache[name];

    GLCall(int location = glGetUniformLocation(m_shader_id, name.c_str()));
    if (location == -1)
        SPDLOG_WARN("Uniform '{}' doesn't exist!", name);
    m_uniform_location_cache[name] = location;
    return location;
}

unsigned int Shader::create_shader(const std::string& vertex_shader, const std::string& fragment_shader) {
    unsigned int program = glCreateProgram();
    unsigned int vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);
    unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);

    if (vs == 0 || fs == 0) {
        SPDLOG_ERROR("Shader compilation failed, abort linking");
        if (vs)
            glDeleteShader(vs);
        if (fs)
            glDeleteShader(fs);
        return 0;
    }

    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);
    glValidateProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

unsigned int Shader::compile_shader(unsigned int type, const std::string& source) {
    if (source.empty()) {
        SPDLOG_ERROR("empty shader source provided for type {}", (type == GL_VERTEX_SHADER ? "vertex" : "fragment"));
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

        const GLubyte* glVersion = glGetString(GL_VERSION);
        const GLubyte* glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION);
        SPDLOG_ERROR("Failed to compile {} shader! OpenGL: {}, GLSL: {}",
                     (type == GL_VERTEX_SHADER ? "vertex" : "fragment"),
                     (glVersion ? reinterpret_cast<const char*>(glVersion) : "unknown"),
                     (glsl_version ? reinterpret_cast<const char*>(glsl_version) : "unknown"));

        SPDLOG_ERROR("Shader compilation error: {}", message);

        SPDLOG_ERROR("Shader source:\n{}", ShaderSource::numbered(source));
        glDeleteShader(id);
        return 0;
    }
    return id;
}

} // namespace vat::gl
