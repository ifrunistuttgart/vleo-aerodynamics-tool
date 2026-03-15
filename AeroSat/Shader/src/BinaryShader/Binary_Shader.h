#pragma once
#include <memory>
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
	const int MAX_TRIANGLES = 65536 - 1;
	size_t m_lenVertices = 0;
	unsigned int m_numTriangles = 0;
	unsigned int m_ID_texture = 0;
	unsigned int m_histogramBuffer = 0;
	const unsigned int NUM_PIXEL = 800;
public:
	BinaryShader(unsigned int num_pixel);
	~BinaryShader();
	int set_vertices(float vertices[], size_t lenVertices, unsigned int triangleIDs[], size_t lenTriangleIDs) override;
	int shade_satellite(float triangle_visibility[], glm::vec3 v_rel_hat, float bounding_sphere_radius) override;
};