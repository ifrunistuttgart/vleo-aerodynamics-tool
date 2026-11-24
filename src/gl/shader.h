#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "glm/glm.hpp"
#include "shader_source.h"

namespace vat::gl {

/**
 * @brief Owns one linked OpenGL program made of a vertex and a fragment shader.
 *
 * A context must be current when the program is created, used and destroyed. Uniform locations
 * are cached by name; setting a uniform the program does not have logs a warning.
 */
class Shader {
private:
    unsigned int m_ShaderID;
    std::string m_FilePath;

    // caching for uniforms
    std::unordered_map<std::string, int> m_UniformLocationCache;

public:
    /**
     * @brief Compiles and links a program from embedded source strings.
     * @param vertexSource Complete GLSL vertex shader source, #version line included.
     * @param fragmentSource Complete GLSL fragment shader source, #version line included.
     */
    Shader(const std::string& vertexSource, const std::string& fragmentSource);
    /**
     * @brief Compiles and links a program from shader templates.
     * @param vertexSource Vertex shader template whose placeholders all have values.
     * @param fragmentSource Fragment shader template whose placeholders all have values.
     * @throws std::logic_error if a placeholder has no value.
     */
    Shader(const ShaderSource& vertexSource, const ShaderSource& fragmentSource);
    ~Shader();

    /** @brief Makes this program current (glUseProgram). */
    void Bind() const;
    /** @brief Makes no program current. */
    void Unbind() const;

    /** @brief Sets an int (or sampler) uniform; the program must be bound. */
    void SetUniform1i(const std::string& name, int value);
    /** @brief Sets a float uniform; the program must be bound. */
    void SetUniform1f(const std::string& name, float value);
    /** @brief Sets a vec3 uniform; the program must be bound. */
    void SetUniform3f(const std::string& name, const glm::vec3& value);
    /**
     * @brief Sets values.size() consecutive elements of a vec2 array, starting at the named one.
     *
     * The program must be bound.
     */
    void SetUniform2fv(const std::string& name, const std::vector<glm::vec2>& values);
    /** @brief Sets a vec4 uniform; the program must be bound. */
    void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
    /** @brief Sets a mat3 uniform (column-major, not transposed); the program must be bound. */
    void setUniformMat3f(const std::string& name, const glm::mat3& matrix);
    /** @brief Sets a mat4 uniform (column-major, not transposed); the program must be bound. */
    void setUniformMat4f(const std::string& name, const glm::mat4& matrix);

private:
    int GetUniformLocation(const std::string& name);
    unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader);
    unsigned int CompileShader(unsigned int type, const std::string& source);
};

} // namespace vat::gl
