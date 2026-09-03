#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "../opengl_abstractions/compute_shader.h"

/**
 * Reduces a rendered triangle-ID image to one visibility value per triangle.
 *
 * A compute shader scans the ID texture on the GPU and marks every ID it finds, so
 * only one flag per triangle has to be read back instead of the whole image. The
 * predicate is the same one a CPU scan of the resolved framebuffer applies: a
 * triangle is visible if at least one pixel carries its ID.
 */
class VisibilityReducer {
public:
	/**
	 * @param num_triangles Total number of triangles; triangle IDs are 1-based, so the
	 *                      flag buffer holds one extra slot for the background (ID 0).
	 */
	explicit VisibilityReducer(unsigned int num_triangles);
	~VisibilityReducer();

	VisibilityReducer(const VisibilityReducer&) = delete;
	VisibilityReducer& operator=(const VisibilityReducer&) = delete;

	/**
	 * @param id_texture R32UI texture holding the rendered triangle IDs.
	 * @param num_pixel Edge length of the square ID texture.
	 * @return 1.0 for every visible triangle, 0.0 otherwise.
	 */
	std::vector<float> reduce(unsigned int id_texture, unsigned int num_pixel);

private:
	std::unique_ptr<ComputeShader> m_shader;
	unsigned int m_visibility_buffer = 0;
	unsigned int m_num_triangles = 0;
	std::vector<std::uint32_t> m_flags; // reused staging buffer for the readback
};
