#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <span>
#include <stdexcept>
#include <type_traits>

#include "gl_helpers.h"
#include "pixel_load_integrator.h"
#include "leeward_classification.h"
#include "memory_barrier.h"
#include "shaders/pixel_loads_glsl.h"

namespace vat::loads {

// The gl:: wrappers are this layer's own building blocks; unqualified use keeps
// the OpenGL call sites readable. TU-local, so it never leaks through a header.
using namespace gl;

namespace {

constexpr unsigned int TILE_SIZE = 32; // pixels per workgroup and axis, see pixel_loads_glsl.h

std::string assemble_integration_shader(const std::string& gsi_glsl, bool keep_pressure_image, unsigned int num_samples) {
	std::string source = "#version 430\n";
	if (keep_pressure_image) {
		source += "#define VAT_WRITE_PRESSURE\n";
	}
	source += "#define VAT_MSAA_SAMPLES " + std::to_string(num_samples) + "\n";
	source += pixel_glsl::integration_header;
	source += gsi_glsl;
	source += pixel_glsl::integration_main;
	return source;
}

} // namespace

PixelLoadIntegrator::PixelLoadIntegrator(const std::string& gsi_glsl, unsigned int num_pixel, bool keep_pressure_image,
                                         unsigned int num_samples)
	: m_shader(std::make_unique<ComputeShader>(assemble_integration_shader(gsi_glsl, keep_pressure_image, num_samples), true)),
	  m_num_pixel(num_pixel),
	  m_num_samples(num_samples),
	  m_num_groups((num_pixel + TILE_SIZE - 1) / TILE_SIZE),
	  m_partials(2 * static_cast<size_t>(m_num_groups) * m_num_groups),
	  m_partials_buffer(m_partials.size() * sizeof(glm::vec4), BufferUsage::DynamicRead) {
	if (keep_pressure_image) {
		m_pressure_texture = std::make_unique<Texture2D>(static_cast<int>(num_pixel), static_cast<int>(num_pixel),
			GL_R32F, GL_RED, GL_FLOAT);
		if (!m_pressure_texture->is_valid()) {
			throw std::runtime_error("PixelLoadIntegrator: could not allocate the pressure image");
		}
	}

	SPDLOG_DEBUG("PixelLoadIntegrator ready for {}x{} pixels, {} sample(s) ({} workgroups, {} bytes read back per call)",
		num_pixel, num_pixel, num_samples, m_num_groups * m_num_groups, m_partials.size() * sizeof(glm::vec4));
}

PixelLoadIntegrator::~PixelLoadIntegrator() = default;

PixelLoadIntegrator::Sums PixelLoadIntegrator::integrate(const Texture2D& position_texture, const Texture2D& normal_texture,
                                                         const glm::vec3& v_rel__m_per_s, float surface_temp__K,
                                                         float pixel_area__m2, float min_cos_delta,
                                                         const std::vector<GlslUniform>& gsi_uniforms,
                                                         std::vector<float>& pressure_image) {
	// Checked first, so a mismatch leaves no GL state behind.
	if (static_cast<unsigned int>(position_texture.get_num_samples()) != m_num_samples ||
	    static_cast<unsigned int>(normal_texture.get_num_samples()) != m_num_samples) {
		throw std::invalid_argument("PixelLoadIntegrator::integrate: G-buffer sample count differs from the integrator's");
	}

	m_shader->Bind();
	m_shader->SetUniform1i("u_g_position", 0);
	m_shader->SetUniform1i("u_g_normal", 1);
	m_shader->SetUniform3f("u_v_rel", v_rel__m_per_s);
	m_shader->SetUniform1f("u_surface_temp", surface_temp__K);
	m_shader->SetUniform1f("u_pixel_area", pixel_area__m2);
	m_shader->SetUniform1f("u_min_cos_delta", min_cos_delta);
	m_shader->SetUniform1f("u_edge_on_threshold", edge_on_threshold(glm::length(v_rel__m_per_s)));
	for (const GlslUniform& uniform : gsi_uniforms) {
		std::visit([&](auto value) {
			if constexpr (std::is_same_v<decltype(value), int>) {
				m_shader->SetUniform1i(uniform.name, value);
			} else {
				m_shader->SetUniform1f(uniform.name, value);
			}
		}, uniform.value);
	}

	position_texture.bind(0);
	normal_texture.bind(1);
	m_partials_buffer.bind_base(0);
	if (m_pressure_texture) {
		m_pressure_texture->bind_as_image(0, GL_WRITE_ONLY);
	}

	m_shader->dispatch(m_num_groups, m_num_groups);
	memory_barrier(Barrier::ShaderStorage | Barrier::BufferUpdate | Barrier::TextureUpdate);
	m_partials_buffer.read_back(std::span<glm::vec4>(m_partials));

	if (m_pressure_texture) {
		pressure_image = m_pressure_texture->read_back_float();
	}

	normal_texture.unbind(1);
	position_texture.unbind(0);
	m_shader->Unbind();

	// Summed in workgroup order, which the dispatch fixes, so the total is reproducible.
	Sums sums;
	for (size_t group = 0; group < m_partials.size() / 2; ++group) {
		const glm::vec4& force = m_partials[2 * group];
		const glm::vec4& torque = m_partials[2 * group + 1];
		sums.force__N += glm::dvec3(force);
		sums.wetted_area__m2 += force.w;
		sums.torque__Nm += glm::dvec3(torque);
		sums.windward_pixels += torque.w;
	}
	// The shader counts windward samples (exact integers); a pixel weighs 1 / num_samples per sample.
	sums.windward_pixels /= static_cast<double>(m_num_samples);
	return sums;
}

} // namespace vat::loads
