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
    unsigned int m_shader_id;
    std::string m_file_path;

    // caching for uniforms
    std::unordered_map<std::string, int> m_uniform_location_cache;

public:
    /**
     * @brief Compiles and links a program from embedded source strings.
     * @param vertex_source Complete GLSL vertex shader source, #version line included.
     * @param fragment_source Complete GLSL fragment shader source, #version line included.
     */
    Shader(const std::string& vertex_source, const std::string& fragment_source);
    /**
     * @brief Compiles and links a program from shader templates.
     * @param vertex_source Vertex shader template whose placeholders all have values.
     * @param fragment_source Fragment shader template whose placeholders all have values.
     * @throws std::logic_error if a placeholder has no value.
     */
    Shader(const ShaderSource& vertex_source, const ShaderSource& fragment_source);
    ~Shader();

    /** @brief Makes this program current (glUseProgram). */
    void bind() const;
    /** @brief Makes no program current. */
    void unbind() const;

    /** @brief Sets an int (or sampler) uniform; the program must be bound. */
    void set_uniform_1i(const std::string& name, int value);
    /** @brief Sets a float uniform; the program must be bound. */
    void set_uniform_1f(const std::string& name, float value);
    /** @brief Sets a vec3 uniform; the program must be bound. */
    void set_uniform_3f(const std::string& name, const glm::vec3& value);
    /**
     * @brief Sets values.size() consecutive elements of a vec2 array, starting at the named one.
     *
     * The program must be bound.
     */
    void set_uniform_2fv(const std::string& name, const std::vector<glm::vec2>& values);
    /** @brief Sets a vec4 uniform; the program must be bound. */
    void set_uniform_4f(const std::string& name, float v0, float v1, float v2, float v3);
    /** @brief Sets a mat3 uniform (column-major, not transposed); the program must be bound. */
    void set_uniform_mat3f(const std::string& name, const glm::mat3& matrix);
    /** @brief Sets a mat4 uniform (column-major, not transposed); the program must be bound. */
    void set_uniform_mat4f(const std::string& name, const glm::mat4& matrix);

private:
    int get_uniform_location(const std::string& name);
    unsigned int create_shader(const std::string& vertex_shader, const std::string& fragment_shader);
    unsigned int compile_shader(unsigned int type, const std::string& source);
};

} // namespace vat::gl
