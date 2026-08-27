#pragma once
#include <memory>
#include <span>
#include <cstdint>
#include <glm/glm.hpp>

#include "opengl/frame_buffer.h"
#include "opengl/shader.h"
#include "opengl/vertex_array.h"
#include "Ishading_algorithm.h"

namespace vat {

class BinaryShader : public IShadingAlgorithm {
private:
	std::unique_ptr<gl::FrameBuffer> m_frame_buffer;
	std::unique_ptr<gl::Shader> m_shader;
	std::unique_ptr<gl::VertexArray> m_vao;
	const unsigned int MAX_TRIANGLES = (2u << 28) - 1; //limit histogrambuffer size to about 1GB
	size_t m_lenVertices = 0;
	unsigned int m_numTriangles = 0;
	unsigned int m_ID_texture = 0;
	const unsigned int NUM_PIXEL = 800;
public:
	BinaryShader(unsigned int num_pixel);
	~BinaryShader();
	int set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) override;
	std::vector<float> shade_satellite(glm::vec3 v_rel_hat, float bounding_sphere_radius, std::span<const unsigned int> num_triangles_per_mesh, std::span<const glm::mat4> model_matrices) override;
};

} // namespace vat
