#pragma once
#include <string>
#include <unordered_map>
#include "glm/glm.hpp"
#include "shader_source.h"

namespace vat::gl {

class ComputeShader {
private:
    unsigned int m_ComputeShaderID;
    std::string m_FilePath;

    // caching for uniforms
    std::unordered_map<std::string, int> m_UniformLocationCache;

public:
    ComputeShader(const std::string& computeSource);
    // Construct from a shader template; throws std::logic_error if a placeholder has no value
    explicit ComputeShader(const ShaderSource& computeSource);
    ~ComputeShader();

    void Bind() const;
    void Unbind() const;

    // Runs the shader on a grid of work groups; it must be bound. Follow with
    // memory_barrier() before reading what the shader wrote.
    void dispatch(unsigned int groups_x, unsigned int groups_y = 1, unsigned int groups_z = 1) const;

    // set uniforms
    void SetUniform1i(const std::string& name, int value);
    void SetUniform1f(const std::string& name, float value);
    void SetUniform3f(const std::string& name, const glm::vec3& value);
    void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
    void setUniformMat4f(const std::string& name, const glm::mat4& matrix);

private:
    int GetUniformLocation(const std::string& name);
    unsigned int CreateShader(const std::string& computeShader);
    unsigned int CompileShader(unsigned int type, const std::string& source);
};

} // namespace vat::gl
