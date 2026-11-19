#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "glm/glm.hpp"
#include "shader_source.h"

namespace vat::gl {

class Shader
{
private:
	unsigned int m_ShaderID;
	std::string m_FilePath;

	//caching for uniforms
	std::unordered_map<std::string, int> m_UniformLocationCache;

public:
	// Construct from embedded/source strings
	Shader(const std::string& vertexSource, const std::string& fragmentSource);
	// Construct from shader templates; throws std::logic_error if a placeholder has no value
	Shader(const ShaderSource& vertexSource, const ShaderSource& fragmentSource);
	~Shader();

	void Bind() const;
	void Unbind() const;

	// set uniforms
	void SetUniform1i(const std::string& name, int value);
	void SetUniform1f(const std::string& name, float value);
	void SetUniform3f(const std::string& name, const glm::vec3& value);
	// Sets values.size() consecutive elements of a vec2 array, starting at the named one.
	void SetUniform2fv(const std::string& name, const std::vector<glm::vec2>& values);
	void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
	void setUniformMat3f(const std::string& name, const glm::mat3& matrix);
	void setUniformMat4f(const std::string& name, const glm::mat4& matrix);

private:
	int GetUniformLocation(const std::string& name);
	unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader);
	unsigned int CompileShader(unsigned int type, const std::string& source);
};

} // namespace vat::gl
