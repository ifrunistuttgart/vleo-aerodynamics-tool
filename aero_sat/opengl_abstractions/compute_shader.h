#pragma once
#include <string>
#include <unordered_map>
#include "glm/glm.hpp"
#include "texture_2d.h"

struct ComputeShaderProgramSource
{
    std::string compute_source;
};

class ComputeShader
{
private:
    unsigned int m_compute_shader_id;
    std::string m_file_path;

    //caching for uniforms
    std::unordered_map<std::string, int> m_uniform_location_cache;

public:
    ComputeShader(const std::string& compute_source, bool from_source);
    ~ComputeShader();

    void bind() const;
    void unbind() const;

    void run(unsigned int num_groups_x, unsigned int num_groups_y, unsigned int num_groups_z);

    // set uniforms
    void set_uniform_4f(const std::string& name, const glm::vec4& vector);
    void set_uniform_3f(const std::string& name, const glm::vec3& vector);
    void set_uniform_2f(const std::string& name, const glm::vec2& vector);
    void set_uniform_1f(const std::string& name, float value);
    void set_uniform_mat4f(const std::string& name, const glm::mat4& matrix);

    // set textures
    void set_texture(unsigned int binding_number, const Texture2D& texture);

private:
    int get_uniform_location(const std::string& name);
    unsigned int create_shader(const std::string& compute_shader);
    unsigned int compile_shader(unsigned int type, const std::string& source);
};