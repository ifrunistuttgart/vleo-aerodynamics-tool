#pragma once
#include <string>
#include <unordered_map>
#include "glm/glm.hpp"

namespace vat::gl {

struct ShaderProgramSource
{
	std::string VertexSource;
	std::string FragmentSource;
};

class Shader
{
private:
	unsigned int m_ShaderID;
	std::string m_FilePath;

	//caching for uniforms
	std::unordered_map<std::string, int> m_UniformLocationCache;

public:
	// Construct from embedded/source strings
	Shader(const std::string& vertexSource, const std::string& fragmentSource, bool fromSource);
	~Shader();

	void Bind() const;
	void Unbind() const;

	// set uniforms
	void SetUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
	void setUniformMat4f(const std::string& name, const glm::mat4& matrix);

private:
	int GetUniformLocation(const std::string& name);
	unsigned int CreateShader(const std::string& vertexShader, const std::string& fragmentShader);
	unsigned int CompileShader(unsigned int type, const std::string& source);
};

} // namespace vat::gl
