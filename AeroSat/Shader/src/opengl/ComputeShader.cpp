#include "ComputeShader.h"

#include <iostream>
#include <string>

#include "src/opengl/GLHelpers.h"

// Constructor: create compute shader directly from provided source string
ComputeShader::ComputeShader(const std::string& computeSource, bool fromSource)
    : m_FilePath(""), m_ComputeShaderID(0)
{
    if (computeSource.empty()) {
        std::cerr << "ComputeShader::ComputeShader - empty source" << std::endl;
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
        std::cout << "warning: uniform " << name << " doesn't exist !" << std::endl;
    m_UniformLocationCache[name] = location;
    return location;
}

unsigned int ComputeShader::CreateShader(const std::string& source)
{
    unsigned int program = glCreateProgram();
    unsigned int cs = CompileShader(GL_COMPUTE_SHADER, source);

    if (cs == 0) {
        std::cerr << "ComputeShader::CreateShader - compute shader compilation failed, abort linking." << std::endl;
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
        std::cerr << "CompileShader: empty source" << std::endl;
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
        std::cerr << "Failed to compile compute shader!\nInfoLog:\n" << message << std::endl;

        const GLubyte* glVersion = glGetString(GL_VERSION);
        const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
        std::cerr << "OpenGL: " << (glVersion ? reinterpret_cast<const char*>(glVersion) : "unknown")
            << "\nGLSL: " << (glslVersion ? reinterpret_cast<const char*>(glslVersion) : "unknown") << std::endl;

        const size_t maxDump = 4096;
        std::cerr << "Compute shader source (truncated to " << maxDump << " chars):\n"
            << source.substr(0, std::min(source.size(), maxDump)) << std::endl;

        glDeleteShader(id);
        return 0;
    }
    return id;
}

