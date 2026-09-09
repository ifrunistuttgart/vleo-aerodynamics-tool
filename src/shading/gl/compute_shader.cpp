#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <string>

#include "gl_helpers.h"
#include "compute_shader.h"

// Constructor: create compute shader directly from provided source string
ComputeShader::ComputeShader(const std::string& computeSource, bool fromSource)
    : m_FilePath(""), m_ComputeShaderID(0)
{
    if (computeSource.empty()) {
		SPDLOG_ERROR("Computeshader source is empty");
        return;
    }
    m_ComputeShaderID = CreateShader(computeSource);
}

ComputeShader::~ComputeShader()
{
    GLCall(glDeleteProgram(m_ComputeShaderID));
}

void ComputeShader::Bind() const
{
    GLCall(glUseProgram(m_ComputeShaderID));
}

void ComputeShader::Unbind() const
{
    GLCall(glUseProgram(0));
}

void ComputeShader::SetUniform1i(const std::string& name, int value)
{
    GLCall(glUniform1i(GetUniformLocation(name), value));
}

void ComputeShader::SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3)
{
    GLCall(glUniform4f(GetUniformLocation(name), v0, v1, v2, v3));
}

void ComputeShader::setUniformMat4f(const std::string& name, const glm::mat4& matrix)
{
    GLCall(glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, &matrix[0][0]));
}

int ComputeShader::GetUniformLocation(const std::string& name)
{
    if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
        return m_UniformLocationCache[name];

    GLCall(int location = glGetUniformLocation(m_ComputeShaderID, name.c_str()));
    if (location == -1)
        SPDLOG_WARN("Uniform {} doesn't exist!", name);
    m_UniformLocationCache[name] = location;
    return location;
}

unsigned int ComputeShader::CreateShader(const std::string& source)
{
    unsigned int program = glCreateProgram();
    unsigned int cs = CompileShader(GL_COMPUTE_SHADER, source);

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

unsigned int ComputeShader::CompileShader(unsigned int type, const std::string& source)
{
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
    if (result == GL_FALSE)
    {
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        std::string message(length, '\0');
        glGetShaderInfoLog(id, length, &length, &message[0]);
        SPDLOG_ERROR("Failed to compile compute shader!\nInfoLog:\n{}", message);

        const GLubyte* glVersion = glGetString(GL_VERSION);
        const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
        SPDLOG_ERROR("OpenGL: {}\nGLSL: {}", glVersion ? reinterpret_cast<const char*>(glVersion) : "unknown",
            glslVersion ? reinterpret_cast<const char*>(glslVersion) : "unknown");

        const size_t maxDump = 4096;
        SPDLOG_ERROR("Compute shader source (truncated to {} chars):\n{}", maxDump,
            source.substr(0, std::min(source.size(), maxDump)));

        glDeleteShader(id);
        return 0;
    }
    return id;
}

