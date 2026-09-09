#include "shader.h"

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <string>

#include "gl_helpers.h"

Shader::Shader(const std::string& vertex_source, const std::string& fragment_source, bool fromSource)
    : m_file_path(""), m_shader_id(0)
{
    m_shader_id = create_shader(vertex_source, fragment_source);
}

Shader::~Shader()
{
    GLCall(glDeleteProgram(m_shader_id));
}

void Shader::bind() const
{
    GLCall(glUseProgram(m_shader_id));
}

void Shader::unbind() const
{
    GLCall(glUseProgram(0));
}

void Shader::set_uniform_4f(const std::string& name, const glm::vec4& vector)
{
    GLCall(glUniform4f(get_uniform_location(name), vector.x, vector.y, vector.z, vector.w));
}

void Shader::set_uniform_3f(const std::string& name, const glm::vec3& vector)
{
    GLCall(glUniform3f(get_uniform_location(name), vector.x, vector.y, vector.z));
}

void Shader::set_uniform_2f(const std::string &name, const glm::vec2 &vector) {
    GLCall(glUniform2f(get_uniform_location(name), vector.x, vector.y));
}

void Shader::set_uniform_1f(const std::string &name, float value) {
    GLCall(glUniform1f(get_uniform_location(name), value));
}

void Shader::set_uniform_mat4f(const std::string& name, const glm::mat4& matrix)
{
    GLCall(glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, &matrix[0][0]));
}

void Shader::set_uniform_mat3f(const std::string& name, const glm::mat3& matrix)
{
    GLCall(glUniformMatrix3fv(get_uniform_location(name), 1, GL_FALSE, &matrix[0][0]));
}

int Shader::get_uniform_location(const std::string& name)
{
    if (m_uniform_location_cache.find(name) != m_uniform_location_cache.end())
        return m_uniform_location_cache[name];

    GLCall(int location = glGetUniformLocation(m_shader_id, name.c_str()));
    if (location == -1)
        SPDLOG_WARN("Uniform '{}' doesn't exist!", name);
    m_uniform_location_cache[name] = location;
    return location;
}

unsigned int Shader::create_shader(const std::string& vertex_shader, const std::string& fragment_shader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = compile_shader(GL_VERTEX_SHADER, vertex_shader);
    unsigned int fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader);

    if (vs == 0 || fs == 0) {
        SPDLOG_ERROR("Shader compilation failed, abort linking");
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
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

unsigned int Shader::compile_shader(unsigned int type, const std::string& source)
{
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
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        std::string message(length, '\0');
        glGetShaderInfoLog(id, length, &length, &message[0]);

        const GLubyte* glVersion = glGetString(GL_VERSION);
        const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
        SPDLOG_ERROR("Failed to compile {} shader! OpenGL: {}, GLSL: {}",
            (type == GL_VERTEX_SHADER ? "vertex" : "fragment"),
            (glVersion ? reinterpret_cast<const char*>(glVersion) : "unknown"),
			(glslVersion ? reinterpret_cast<const char*>(glslVersion) : "unknown"));

		SPDLOG_ERROR("Shader compilation error: {}", message);

        const size_t maxDump = 4096;
        SPDLOG_ERROR("Shader source (truncated to {} chars):\n{}", maxDump, source.substr(0, std::min(source.size(), maxDump)));
        glDeleteShader(id);
        return 0;
    }
    return id;
}

