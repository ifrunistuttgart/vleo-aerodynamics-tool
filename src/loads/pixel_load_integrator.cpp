#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <type_traits>

#include "gl_helpers.h"
#include "pixel_load_integrator.h"
#include "shaders/pixel_loads_glsl.h"

namespace vat::loads {

// The gl:: wrappers are this layer's own building blocks; unqualified use keeps
// the OpenGL call sites readable. TU-local, so it never leaks through a header.
using namespace gl;

namespace {

constexpr unsigned int TILE_SIZE = 32; // pixels per workgroup and axis, see pixel_loads_glsl.h

std::string assemble_integration_shader(const std::string& gsi_glsl, bool keep_pressure_image) {
	std::string source = "#version 430\n";
	if (keep_pressure_image) {
		source += "#define VAT_WRITE_PRESSURE\n";
	}
	source += pixel_glsl::integration_header;
	source += gsi_glsl;
	source += pixel_glsl::integration_main;
	return source;
}

} // namespace

PixelLoadIntegrator::PixelLoadIntegrator(const std::string& gsi_glsl, unsigned int num_pixel, bool keep_pressure_image)
	: m_shader(std::make_unique<ComputeShader>(assemble_integration_shader(gsi_glsl, keep_pressure_image), true)),
	  m_num_pixel(num_pixel),
	  m_num_groups((num_pixel + TILE_SIZE - 1) / TILE_SIZE),
	  m_partials(2 * static_cast<size_t>(m_num_groups) * m_num_groups) {
	GLCall(glGenBuffers(1, &m_partials_buffer));
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_partials_buffer));
	GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER,
		static_cast<GLsizeiptr>(m_partials.size() * sizeof(glm::vec4)), nullptr, GL_DYNAMIC_READ));
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));

	if (keep_pressure_image) {
		GLCall(glGenTextures(1, &m_pressure_texture));
		GLCall(glBindTexture(GL_TEXTURE_2D, m_pressure_texture));
		GLCall(glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, num_pixel, num_pixel));
		GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	}

	SPDLOG_DEBUG("PixelLoadIntegrator ready for {}x{} pixels ({} workgroups, {} bytes read back per call)",
		num_pixel, num_pixel, m_num_groups * m_num_groups, m_partials.size() * sizeof(glm::vec4));
}

PixelLoadIntegrator::~PixelLoadIntegrator() {
	if (m_partials_buffer != 0) {
		GLCall(glDeleteBuffers(1, &m_partials_buffer));
	}
	if (m_pressure_texture != 0) {
		GLCall(glDeleteTextures(1, &m_pressure_texture));
	}
}

PixelLoadIntegrator::Sums PixelLoadIntegrator::integrate(unsigned int position_texture, unsigned int normal_texture,
                                                         const glm::vec3& v_rel__m_per_s, float surface_temp__K,
                                                         float pixel_area__m2, float min_cos_delta,
                                                         const std::vector<GlslUniform>& gsi_uniforms,
                                                         std::vector<float>& pressure_image) {
	m_shader->Bind();
	m_shader->SetUniform1i("u_g_position", 0);
	m_shader->SetUniform1i("u_g_normal", 1);
	m_shader->SetUniform3f("u_v_rel", v_rel__m_per_s);
	m_shader->SetUniform1f("u_surface_temp", surface_temp__K);
	m_shader->SetUniform1f("u_pixel_area", pixel_area__m2);
	m_shader->SetUniform1f("u_min_cos_delta", min_cos_delta);
	for (const GlslUniform& uniform : gsi_uniforms) {
		std::visit([&](auto value) {
			if constexpr (std::is_same_v<decltype(value), int>) {
				m_shader->SetUniform1i(uniform.name, value);
			} else {
				m_shader->SetUniform1f(uniform.name, value);
			}
		}, uniform.value);
	}

	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, position_texture));
	GLCall(glActiveTexture(GL_TEXTURE1));
	GLCall(glBindTexture(GL_TEXTURE_2D, normal_texture));
	GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_partials_buffer));
	if (m_pressure_texture != 0) {
		GLCall(glBindImageTexture(0, m_pressure_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F));
	}

	GLCall(glDispatchCompute(m_num_groups, m_num_groups, 1));
	GLCall(glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT | GL_TEXTURE_UPDATE_BARRIER_BIT));

	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_partials_buffer));
	GLCall(glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
		static_cast<GLsizeiptr>(m_partials.size() * sizeof(glm::vec4)), m_partials.data()));
	GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));

	if (m_pressure_texture != 0) {
		pressure_image.resize(static_cast<size_t>(m_num_pixel) * m_num_pixel);
		GLCall(glBindTexture(GL_TEXTURE_2D, m_pressure_texture));
		GLCall(glPixelStorei(GL_PACK_ALIGNMENT, 4));
		GLCall(glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, pressure_image.data()));
	}

	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
	GLCall(glActiveTexture(GL_TEXTURE0));
	GLCall(glBindTexture(GL_TEXTURE_2D, 0));
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
