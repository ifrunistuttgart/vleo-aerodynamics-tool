#include "shader.h"

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <string>

#include "gl_helpers.h"

namespace vat::gl {

Shader::Shader(const std::string& vertexSource, const std::string& fragmentSource, bool fromSource)
    : m_FilePath(""), m_ShaderID(0)
{
    m_ShaderID = CreateShader(vertexSource, fragmentSource);
}

Shader::~Shader()
{
    GLCall(glDeleteProgram(m_ShaderID));
}

void Shader::Bind() const
{
    GLCall(glUseProgram(m_ShaderID));
}

void Shader::Unbind() const
{
    GLCall(glUseProgram(0));
}

void Shader::SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3)
{
    GLCall(glUniform4f(GetUniformLocation(name), v0, v1, v2, v3));
}

void Shader::setUniformMat4f(const std::string& name, const glm::mat4& matrix)
{
    GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]));
}

int Shader::GetUniformLocation(const std::string& name)
{
    if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
        return m_UniformLocationCache[name];

    GLCall(int location = glGetUniformLocation(m_ShaderID, name.c_str()));
    if (location == -1)
        SPDLOG_WARN("Uniform '{}' doesn't exist!", name);
    m_UniformLocationCache[name] = location;
    return location;
}

unsigned int Shader::CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

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

unsigned int Shader::CompileShader(unsigned int type, const std::string& source)
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

} // namespace vat::gl
