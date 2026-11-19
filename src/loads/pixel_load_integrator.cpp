#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <array>
#include <span>
#include <stdexcept>

#include "gl_helpers.h"
#include "pixel_load_integrator.h"
#include "memory_barrier.h"
#include "shaders/pixel_loads_glsl.h"

namespace vat::loads {

// The gl:: wrappers are this layer's own building blocks; unqualified use keeps
// the OpenGL call sites readable. TU-local, so it never leaks through a header.
using namespace gl;

namespace {

// Pixels per workgroup and axis (a 2x2 block per invocation), see pixel_loads_glsl.h.
constexpr unsigned int k_tile_size = 32;

std::string assemble_integration_shader(bool keep_pressure_image) {
	std::string source = "#version 430\n";
	if (keep_pressure_image) {
		source += "#define VAT_WRITE_PRESSURE\n";
	}
	source += pixel_glsl::integration_header;
	source += pixel_glsl::integration_main;
	return source;
}

} // namespace

PixelLoadIntegrator::PixelLoadIntegrator(unsigned int num_pixel, bool keep_pressure_image)
	: m_shader(std::make_unique<ComputeShader>(assemble_integration_shader(keep_pressure_image), true)),
	  m_num_pixel(num_pixel),
	  m_num_groups((num_pixel + k_tile_size - 1) / k_tile_size),
	  m_partials(2 * static_cast<size_t>(m_num_groups) * m_num_groups),
	  m_partials_buffer(m_partials.size() * sizeof(glm::vec4), BufferUsage::DynamicRead) {
	if (keep_pressure_image) {
		m_pressure_texture = std::make_unique<Texture2D>(static_cast<int>(num_pixel), static_cast<int>(num_pixel),
			GL_R32F, GL_RED, GL_FLOAT);
		if (!m_pressure_texture->is_valid()) {
			throw std::runtime_error("PixelLoadIntegrator: could not allocate the pressure image");
		}
	}

	SPDLOG_DEBUG("PixelLoadIntegrator ready for {}x{} pixels ({} workgroups, {} bytes read back per call)",
		num_pixel, num_pixel, m_num_groups * m_num_groups, m_partials.size() * sizeof(glm::vec4));
}

PixelLoadIntegrator::~PixelLoadIntegrator() = default;

size_t PixelLoadIntegrator::num_g_buffer_textures() const {
	return m_pressure_texture ? 3 : 2;
}

PixelLoadIntegrator::Sums PixelLoadIntegrator::integrate(std::span<const std::shared_ptr<Texture2D>> g_buffer,
                                                         float pixel_area__m2, std::vector<float>& pressure_image) {
	// Checked first, so a mismatch leaves no GL state behind. A multisampled G-buffer must be
	// resolved before it is integrated.
	if (g_buffer.size() != num_g_buffer_textures()) {
		throw std::invalid_argument("PixelLoadIntegrator::integrate: wrong number of G-buffer textures");
	}
	for (const auto& texture : g_buffer) {
		if (!texture || texture->is_multisample()) {
			throw std::invalid_argument("PixelLoadIntegrator::integrate: the G-buffer must be single-sample (resolve it first)");
		}
	}

	m_shader->Bind();
	static constexpr std::array<const char*, 3> samplers{"u_g_force", "u_g_torque", "u_g_pressure"};
	for (size_t unit = 0; unit < g_buffer.size(); ++unit) {
		m_shader->SetUniform1i(samplers[unit], static_cast<int>(unit));
	}
	m_shader->SetUniform1f("u_pixel_area", pixel_area__m2);

	for (size_t unit = 0; unit < g_buffer.size(); ++unit) {
		g_buffer[unit]->bind(static_cast<unsigned int>(unit));
	}
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

	for (size_t unit = g_buffer.size(); unit-- > 0;) {
		g_buffer[unit]->unbind(static_cast<unsigned int>(unit));
	}
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
	return sums;
}

} // namespace vat::loads
