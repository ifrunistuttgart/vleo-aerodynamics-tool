#pragma once
#include <memory>
#include <span>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "compute_shader.h"
#include "shader_storage_buffer.h"
#include "texture_2d.h"
#include "Igsi_model.h"

namespace vat::loads {

/**
 * Sums the loads over the pixels of a G-buffer on the GPU.
 *
 * Without MSAA the G-buffer holds position and normal, and the GSI model is evaluated in
 * every pixel here. With MSAA it holds the resolved loads, which the G-buffer pass already
 * evaluated (see pixel_loads_glsl.h), and they are only scaled and summed.
 *
 * One compute dispatch does both: each pixel's force and torque, then a fixed-order
 * tree reduction to one partial sum per tile (32x32 pixels, 16x16 with MSAA). Only the partial sums are read
 * back, and they are added up in double in a fixed order, so the result is
 * reproducible bit for bit on a given GPU and driver.
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
	 * @param gsi_glsl The model's gsi_force_per_projected_area() source.
	 * @param num_pixel Edge length of the square G-buffer.
	 * @param keep_pressure_image Also write and read back the per-pixel pressure.
	 * @param num_samples Samples per pixel the G-buffer was rendered with. With > 1 the
	 *                    integrator expects the resolved loads of the MSAA G-buffer pass.
	 */
	PixelLoadIntegrator(const std::string& gsi_glsl, unsigned int num_pixel, bool keep_pressure_image,
	                    unsigned int num_samples = 1);
	~PixelLoadIntegrator();

	PixelLoadIntegrator(const PixelLoadIntegrator&) = delete;
	PixelLoadIntegrator& operator=(const PixelLoadIntegrator&) = delete;

	/** Number of G-buffer textures integrate() expects, see there. */
	[[nodiscard]] size_t num_g_buffer_textures() const;

	/**
	 * @param g_buffer Single-sample RGBA32F textures, num_g_buffer_textures() of them.
	 *                 Without MSAA: body-frame positions (w = 1 where covered) and normals.
	 *                 With MSAA, resolved: (f, coverage), (r x f, wetted-area weight) and,
	 *                 with the pressure image, (pressure) per pixel; see pixel_loads_glsl.h.
	 * @param pressure_image Receives num_pixel^2 values, row 0 at the bottom, if the
	 *                       integrator keeps the pressure image; untouched otherwise.
	 * @throws std::invalid_argument if the number of textures is wrong or one is multisampled.
	 */
	Sums integrate(std::span<const std::shared_ptr<gl::Texture2D>> g_buffer,
	               const glm::vec3& v_rel__m_per_s, float surface_temp__K,
	               float pixel_area__m2, float min_cos_delta,
	               const std::vector<GlslUniform>& gsi_uniforms,
	               std::vector<float>& pressure_image);

private:
	std::unique_ptr<gl::ComputeShader> m_shader;
	unsigned int m_num_pixel;
	bool m_msaa; // the G-buffer holds resolved loads, not geometry
	unsigned int m_num_groups; // per axis
	std::vector<glm::vec4> m_partials; // reused staging buffer for the readback
	gl::ShaderStorageBuffer m_partials_buffer; // one force and one torque vec4 per workgroup
	std::unique_ptr<gl::Texture2D> m_pressure_texture; // R32F, only if the pressure image is kept
};

} // namespace vat::loads
