#pragma once
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "compute_shader.h"
#include "Igsi_model.h"

namespace vat::loads {

/**
 * Evaluates a GSI model in every pixel of a G-buffer and sums the loads on the GPU.
 *
 * One compute dispatch does both: each pixel's force and torque, then a fixed-order
 * tree reduction to one partial sum per 32x32 tile. Only the partial sums are read
 * back, and they are added up in double in a fixed order, so the result is
 * reproducible bit for bit on a given GPU and driver.
 */
class PixelLoadIntegrator {
public:
	struct Sums {
		glm::dvec3 force__N{0.0};
		glm::dvec3 torque__Nm{0.0};
		double wetted_area__m2 = 0.0;   // sum of A_px / max(cos_delta, min_cos_delta)
		double windward_pixels = 0.0;   // number of pixels that carry a load
	};

	/**
	 * @param gsi_glsl The model's gsi_force_per_projected_area() source.
	 * @param num_pixel Edge length of the square G-buffer.
	 * @param keep_pressure_image Also write and read back the per-pixel pressure.
	 */
	PixelLoadIntegrator(const std::string& gsi_glsl, unsigned int num_pixel, bool keep_pressure_image);
	~PixelLoadIntegrator();

	PixelLoadIntegrator(const PixelLoadIntegrator&) = delete;
	PixelLoadIntegrator& operator=(const PixelLoadIntegrator&) = delete;

	/**
	 * @param position_texture RGBA32F body-frame positions, w = 1 where covered.
	 * @param normal_texture RGBA32F body-frame unit normals.
	 * @param pressure_image Receives num_pixel^2 values, row 0 at the bottom, if the
	 *                       integrator keeps the pressure image; untouched otherwise.
	 */
	Sums integrate(unsigned int position_texture, unsigned int normal_texture,
	               const glm::vec3& v_rel__m_per_s, float surface_temp__K,
	               float pixel_area__m2, float min_cos_delta,
	               const std::vector<GlslUniform>& gsi_uniforms,
	               std::vector<float>& pressure_image);

private:
	std::unique_ptr<gl::ComputeShader> m_shader;
	unsigned int m_num_pixel;
	unsigned int m_num_groups; // per axis
	unsigned int m_partials_buffer = 0;
	unsigned int m_pressure_texture = 0;
	std::vector<glm::vec4> m_partials; // reused staging buffer for the readback
};

} // namespace vat::loads
