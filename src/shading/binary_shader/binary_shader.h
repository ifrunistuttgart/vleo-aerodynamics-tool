#pragma once
#include <memory>
#include <span>
#include <cstdint>
#include <glm/glm.hpp>

#include "frame_buffer.h"
#include "shader.h"
#include "visibility_reducer.h"
#include "vertex_array.h"
#include "Ishading_algorithm.h"

namespace vat::shading {

/**
 * Binary visibility: a triangle is visible (1.0) if any of its fragments survives the
 * depth test, otherwise 0.0.
 *
 * Two passes over a depth-only framebuffer:
 *   1. depth pre-pass (GL_LESS, depth writes on),
 *   2. marking pass (GL_LEQUAL, depth writes off): the fragment shader sets
 *      visible[id] = 1 for every fragment that passes the depth test.
 * Every unoccluded fragment counts, so there is no "one ID per pixel" contest between
 * small triangles, and no ID texture or reduction pass is needed.
 *
 * num_msaa_samples sets the anti-aliasing:
 *   - 1 (default): no anti-aliasing. A triangle produces a fragment only if it covers a
 *     pixel centre, so sub-pixel triangles (e.g. windward faces at grazing flow) can be lost.
 *   - > 1: MSAA with num_msaa_samples coverage samples per pixel. A triangle only has to cover
 *     one of them; the fragment shader still runs once per pixel and triangle. Clamped to
 *     what the driver supports (with a warning). The depth buffer takes
 *     num_pixel^2 * num_msaa_samples * 4 bytes.
 *
 * Known limits:
 *   - Slivers narrower than the sample spacing (about 1/sqrt(num_msaa_samples) px) that are
 *     also aligned with the pixel grid can still fall between all samples.
 *   - Back faces are culled.
 */
class BinaryShader : public IShadingAlgorithm {
private:
	std::unique_ptr<gl::FrameBuffer> m_frame_buffer;
	std::unique_ptr<gl::Shader> m_shader;
	std::unique_ptr<gl::VertexArray> m_vao;
	std::unique_ptr<VisibilityReducer> m_visibility;
	const unsigned int MAX_TRIANGLES = (2u << 28) - 1; //keeps the per-triangle visibility buffer to a sane size
	unsigned int m_numTriangles = 0;
	unsigned int m_num_msaa_samples = 0;
	const unsigned int REQUESTED_MSAA_SAMPLES;
	const unsigned int NUM_PIXEL = 800;

	void draw_meshes(const glm::mat4& view_projection, std::span<const unsigned int> num_triangles_per_mesh,
		std::span<const glm::mat4> model_matrices);
public:
	/**
	 * @param num_pixel   Edge length of the square render target.
	 * @param num_msaa_samples Samples per pixel; 1 means no anti-aliasing, > 1 enables MSAA.
	 */
	explicit BinaryShader(unsigned int num_pixel, unsigned int num_msaa_samples = 1);
	~BinaryShader() override;
	int set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) override;
	std::vector<float> shade_geometry(glm::vec3 v_rel_hat, float bounding_sphere_radius, std::span<const unsigned int> num_triangles_per_mesh, std::span<const glm::mat4> model_matrices) override;

	// Samples per pixel actually used, after clamping to the driver limits; 0 before set_vertices().
	unsigned int get_num_msaa_samples() const { return m_num_msaa_samples; }
};

} // namespace vat::shading
