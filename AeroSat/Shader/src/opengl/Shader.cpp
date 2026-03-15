#include "Shader.h"
#include <iostream>
#include <string>
#include "GLHelpers.h"

// Constructor: create shader directly from provided source strings
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
        std::cout << "warning: uniform " << name << " doesn't exist !" << std::endl;
    m_UniformLocationCache[name] = location;
    return location;
}

unsigned int Shader::CreateShader(const std::string& vertexShader, const std::string& fragmentShader)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexShader);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentShader);

    if (vs == 0 || fs == 0) {
        std::cerr << "Shader::CreateShader - shader compilation failed, abort linking." << std::endl;
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

        const GLubyte* glVersion = glGetString(GL_VERSION);
        const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
        std::cerr << "Failed to compile " << (type == GL_VERTEX_SHADER ? "vertex" : "fragment")
            << " shader!\nOpenGL: " << (glVersion ? reinterpret_cast<const char*>(glVersion) : "unknown")
            << "\nGLSL: " << (glslVersion ? reinterpret_cast<const char*>(glslVersion) : "unknown") << std::endl;

        std::cerr << "InfoLog:\n" << message << std::endl;

        const size_t maxDump = 4096;
        std::cerr << "Shader source (truncated to " << maxDump << " chars):\n"
            << source.substr(0, std::min(source.size(), maxDump)) << std::endl;

        glDeleteShader(id);
        return 0;
    }
    return id;
}

