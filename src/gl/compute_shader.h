#pragma once
#include <string>
#include <unordered_map>
#include "glm/glm.hpp"
#include "shader_source.h"

namespace vat::gl {

/**
 * @brief Owns one linked OpenGL compute shader program.
 *
 * A context must be current when the program is created, used and destroyed. Uniform locations
 * are cached by name; setting a uniform the program does not have logs a warning.
 */
class ComputeShader {
private:
    unsigned int m_compute_shader_id;
    std::string m_file_path;

    // caching for uniforms
    std::unordered_map<std::string, int> m_uniform_location_cache;

public:
    /**
     * @brief Compiles and links a compute shader.
     * @param compute_source Complete GLSL source, #version line included.
     */
    ComputeShader(const std::string& compute_source);
    /**
     * @brief Compiles and links a compute shader from a shader template.
     * @param compute_source Template whose placeholders all have values.
     * @throws std::logic_error if a placeholder has no value.
     */
    explicit ComputeShader(const ShaderSource& compute_source);
    ~ComputeShader();

    /** @brief Makes this program current (glUseProgram). */
    void bind() const;
    /** @brief Makes no program current. */
    void unbind() const;

    /**
     * @brief Runs the shader on a grid of work groups; it must be bound.
     *
     * Follow with memory_barrier() before reading what the shader wrote.
     *
     * @param groups_x Number of work groups along x.
     * @param groups_y Number of work groups along y.
     * @param groups_z Number of work groups along z.
     */
    void dispatch(unsigned int groups_x, unsigned int groups_y = 1, unsigned int groups_z = 1) const;

    /** @brief Sets an int (or sampler) uniform; the program must be bound. */
    void set_uniform_1i(const std::string& name, int value);
    /** @brief Sets a float uniform; the program must be bound. */
    void set_uniform_1f(const std::string& name, float value);
    /** @brief Sets a vec3 uniform; the program must be bound. */
    void set_uniform_3f(const std::string& name, const glm::vec3& value);
    /** @brief Sets a vec4 uniform; the program must be bound. */
    void set_uniform_4f(const std::string& name, float v0, float v1, float v2, float v3);
    /** @brief Sets a mat4 uniform (column-major, not transposed); the program must be bound. */
    void set_uniform_mat4f(const std::string& name, const glm::mat4& matrix);

private:
    int get_uniform_location(const std::string& name);
    unsigned int create_shader(const std::string& compute_shader);
    unsigned int compile_shader(unsigned int type, const std::string& source);
};

} // namespace vat::gl
