#pragma once
#include <memory>
#include <span>
#include <cstdint>
#include <glm/glm.hpp>
#include "src/BinaryShader/FrameBuffer.h"
#include "src/opengl/Shader.h"
#include "src/opengl/ComputeShader.h"
#include "src/opengl/VertexArray.h"
#include "src/IShading_Algorithm.h"

class BinaryShader : public IShadingAlgorithm {
private:
	std::unique_ptr<FrameBuffer> m_frame_buffer;
	std::unique_ptr<Shader> m_shader;
	std::unique_ptr<ComputeShader> m_compute_shader;
	std::unique_ptr<VertexArray> m_vao;
	const unsigned int MAX_TRIANGLES = (2u << 28) - 1; //limit histogrambuffer size to about 1GB
	size_t m_lenVertices = 0;
	unsigned int m_numTriangles = 0;
	unsigned int m_ID_texture = 0;
	unsigned int m_histogramBuffer = 0;
	const unsigned int NUM_PIXEL = 800;
public:
	BinaryShader(unsigned int num_pixel);
	~BinaryShader();
	int set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) override;
	int shade_satellite(std::span<float> triangle_visibility, glm::vec3 v_rel_hat, float bounding_sphere_radius, std::span<const unsigned int> num_triangles_per_mesh, std::span<const glm::mat4> model_matrices) override;
};