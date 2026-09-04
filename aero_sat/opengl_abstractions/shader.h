#pragma once
#include <string>
#include <unordered_map>
#include "glm/glm.hpp"

struct ShaderProgramSource
{
	std::string vertex_source;
	std::string fragment_source;
};

class Shader
{
private:
	unsigned int m_shader_id;
	std::string m_file_path;

	//caching for uniforms
	std::unordered_map<std::string, int> m_uniform_location_cache;

public:
	// Construct from embedded/source strings
	Shader(const std::string& vertex_source, const std::string& fragment_source, bool fromSource);
	~Shader();

	void bind() const;
	void unbind() const;

	// set uniforms
	void set_uniform_4f(const std::string& name, const glm::vec4& vector);
	void set_uniform_3f(const std::string& name, const glm::vec3& vector);
	void set_uniform_2f(const std::string& name, const glm::vec2& vector);
	void set_uniform_1f(const std::string& name, float value);
	void set_uniform_mat4f(const std::string& name, const glm::mat4& matrix);
	void set_uniform_mat3f(const std::string& name, const glm::mat3& matrix);



private:
	int get_uniform_location(const std::string& name);
	unsigned int create_shader(const std::string& vertex_shader, const std::string& fragment_shader);
	unsigned int compile_shader(unsigned int type, const std::string& source);
};