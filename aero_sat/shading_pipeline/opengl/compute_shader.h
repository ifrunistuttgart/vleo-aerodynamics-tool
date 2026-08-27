#pragma once
#include <string>
#include <unordered_map>
#include "glm/glm.hpp"

namespace vat::gl {

struct ComputeShaderProgramSource
{
    std::string ComputeSource;
};

class ComputeShader
{
private:
    unsigned int m_ComputeShaderID;
    std::string m_FilePath;

    //caching for uniforms
    std::unordered_map<std::string, int> m_UniformLocationCache;

public:
    ComputeShader(const std::string& computeSource, bool fromSource);
    ~ComputeShader();

    void Bind() const;
    void Unbind() const;

    // set uniforms
    void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
    void setUniformMat4f(const std::string& name, const glm::mat4& matrix);

private:
    int GetUniformLocation(const std::string& name);
    unsigned int CreateShader(const std::string& compuetShader);
    unsigned int CompileShader(unsigned int type, const std::string& source);
};

} // namespace vat::gl
