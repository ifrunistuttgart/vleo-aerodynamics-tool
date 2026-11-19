#pragma once
#include <memory>
#include <span>
#include <vector>
#include <glm/glm.hpp>

#include "compute_shader.h"
#include "shader_storage_buffer.h"
#include "texture_2d.h"

namespace vat::loads {

/**
 * Sums the loads over the pixels of a G-buffer on the GPU.
 *
 * The G-buffer holds the per-pixel loads, which the G-buffer pass already evaluated (see
 * pixel_loads_glsl.h; with MSAA resolved to the sample mean), and they are only scaled and summed.
 *
 * One compute dispatch scales each pixel's force and torque and reduces them in a fixed-order
 * tree to one partial sum per 32x32 pixel tile. Only the partial sums are read back, and they
 * are added up in double in a fixed order, so the result is reproducible bit for bit on a
 * given GPU and driver.
 */
class PixelLoadIntegrator {
public:
	struct Sums {
		glm::dvec3 force__N{0.0};
		glm::dvec3 torque__Nm{0.0};
		double wetted_area__m2 = 0.0;   // sum of coverage * A_px / max(cos_delta, min_cos_delta)
		double windward_pixels = 0.0;   // windward pixels, each weighted by its coverage
	};

	/**
	 * @param num_pixel Edge length of the square G-buffer.
	 * @param keep_pressure_image Also write and read back the per-pixel pressure.
	 */
	PixelLoadIntegrator(unsigned int num_pixel, bool keep_pressure_image);
	~PixelLoadIntegrator();

	PixelLoadIntegrator(const PixelLoadIntegrator&) = delete;
	PixelLoadIntegrator& operator=(const PixelLoadIntegrator&) = delete;

	/** Number of G-buffer textures integrate() expects, see there. */
	[[nodiscard]] size_t num_g_buffer_textures() const;

	/**
	 * @param g_buffer Single-sample RGBA32F textures, num_g_buffer_textures() of them.
	 *                 (f, coverage), (r x f, wetted-area weight) and, with the pressure
	 *                 image, (pressure) per pixel; see pixel_loads_glsl.h.
	 * @param pixel_area__m2 Area of one pixel in the flow plane.
	 * @param pressure_image Receives num_pixel^2 values, row 0 at the bottom, if the
	 *                       integrator keeps the pressure image; untouched otherwise.
	 * @throws std::invalid_argument if the number of textures is wrong or one is multisampled.
	 */
	Sums integrate(std::span<const std::shared_ptr<gl::Texture2D>> g_buffer, float pixel_area__m2,
	               std::vector<float>& pressure_image);

private:
	std::unique_ptr<gl::ComputeShader> m_shader;
	unsigned int m_num_pixel;
	unsigned int m_num_groups; // per axis
	std::vector<glm::vec4> m_partials; // reused staging buffer for the readback
	gl::ShaderStorageBuffer m_partials_buffer; // one force and one torque vec4 per workgroup
	std::unique_ptr<gl::Texture2D> m_pressure_texture; // R32F, only if the pressure image is kept
};

} // namespace vat::loads
