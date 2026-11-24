#include <algorithm>
#include <cmath>
#include <limits>
#include "pixel_force_torque_calculator.h"
#include "leeward_classification.h"
#include <array>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <glm/glm.hpp>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

// custom abstractions
#include "flow_camera.h"
#include "frame_buffer.h"
#include "gl_helpers.h"
#include "glfw_opengl_context.h"
#include "memory_barrier.h"
#include "scoped_context.h"
#include "shader.h"
#include "texture_2d.h"
#include "vertex_array.h"
#include "vertex_buffer.h"
#include "vertex_buffer_layout.h"
#include "pixel_load_integrator.h"

// embedded shader headers
#include "shaders/pixel_loads_glsl.h"

namespace vat::loads {

// The gl:: wrappers are this layer's own building blocks; unqualified use keeps
// the OpenGL call sites readable. TU-local, so it never leaks through a header.
using namespace gl;

// Everything that lives in the OpenGL context. The context is declared first so it is
// destroyed last, after every object that owns GL names in it.
struct PixelForceTorqueCalculator::GpuState {
    std::unique_ptr<GlfwOpenGLContext> context;
    // Force, torque and, with the pressure image, pressure, see pixel_loads_glsl.h.
    std::vector<std::shared_ptr<Texture2D>> g_buffer_textures;
    std::unique_ptr<FrameBuffer> g_buffer;
    // With MSAA: the G-buffer resolved (sample mean) into single-sample textures.
    std::vector<std::shared_ptr<Texture2D>> resolved_textures;
    std::unique_ptr<FrameBuffer> resolved;
    std::unique_ptr<Shader> g_buffer_shader;
    std::unique_ptr<VertexArray> vao;
    std::unique_ptr<PixelLoadIntegrator> integrator;

    ~GpuState() {
        if (!context) {
            return;
        }
        context->make_current();
        integrator.reset();
        resolved.reset();
        for (auto& texture : resolved_textures) {
            texture.reset();
        }
        vao.reset();
        g_buffer_shader.reset();
        g_buffer.reset();
        for (auto& texture : g_buffer_textures) {
            texture.reset();
        }
    }
};

namespace {

// `num_targets` RGBA32F targets, sampled texel by texel and never filtered, with
// `num_samples` samples per pixel (1 = ordinary textures), plus the framebuffer over them.
// Returns false, with nothing left allocated, if the memory is not available.
bool create_g_buffer(std::vector<std::shared_ptr<Texture2D>>& g_buffer_textures, std::unique_ptr<FrameBuffer>& g_buffer,
                     unsigned int num_pixel, unsigned int num_samples, size_t num_targets) {
    const int size = static_cast<int>(num_pixel);
    bool allocated = true;
    g_buffer_textures.assign(num_targets, nullptr);
    for (auto& texture : g_buffer_textures) {
        texture = std::make_shared<Texture2D>(size, size, GL_RGBA32F, GL_RGBA, GL_FLOAT, static_cast<int>(num_samples));
        allocated = allocated && texture->is_valid();
    }

    if (allocated) {
        g_buffer = std::make_unique<FrameBuffer>(num_pixel, num_pixel, num_samples);
        g_buffer->attach_color_textures(g_buffer_textures);
        g_buffer->UnBind();
        if (g_buffer->is_complete()) {
            return true;
        }
    }
    g_buffer.reset();
    g_buffer_textures.clear();
    return false;
}

} // namespace

PixelForceTorqueCalculator::PixelForceTorqueCalculator(IGeometryShadingData& geometry, IGSIModel& gsi_model,
                                                       unsigned int num_pixel, unsigned int num_msaa_samples,
                                                       bool keep_pressure_image, float min_cos_delta,
                                                       FlowRegime flow_regime)
    : m_geometry(geometry), m_gsi_model(gsi_model), m_num_pixel(num_pixel), m_keep_pressure_image(keep_pressure_image),
      m_min_cos_delta(min_cos_delta), m_flow_regime(flow_regime), m_gpu(std::make_unique<GpuState>()) {
    const std::string gsi_glsl = m_gsi_model.glsl_force_per_projected_area();
    if (gsi_glsl.empty()) {
        SPDLOG_ERROR("PixelForceTorqueCalculator: the GSI model has no GPU implementation");
        throw std::invalid_argument("PixelForceTorqueCalculator: the GSI model has no GPU implementation");
    }
    if (num_pixel == 0) {
        SPDLOG_ERROR("PixelForceTorqueCalculator: num_pixel must be positive");
        throw std::invalid_argument("PixelForceTorqueCalculator: num_pixel must be positive");
    }
    if (num_msaa_samples == 0) {
        SPDLOG_ERROR("PixelForceTorqueCalculator: num_msaa_samples must be at least 1 (1 = no anti-aliasing)");
        throw std::invalid_argument(
            "PixelForceTorqueCalculator: num_msaa_samples must be at least 1 (1 = no anti-aliasing)");
    }

    // The window is never shown and never drawn to; all rendering goes to the G-buffer.
    m_gpu->context = std::make_unique<GlfwOpenGLContext>(64, 64, "Pixel Load Renderer", false);
    ScopedCurrentContext current(*m_gpu->context);

    // The G-buffer has multisampled colour textures and a multisampled depth renderbuffer,
    // so both limits apply.
    GLint max_color_samples = 1;
    GLint max_samples = 1;
    GLCall(glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &max_color_samples));
    GLCall(glGetIntegerv(GL_MAX_SAMPLES, &max_samples));
    const auto driver_limit = static_cast<unsigned int>(std::max(1, std::min(max_color_samples, max_samples)));
    m_num_msaa_samples = std::min(num_msaa_samples, driver_limit);
    if (m_num_msaa_samples < num_msaa_samples) {
        SPDLOG_WARN("Requested {} MSAA samples, the driver allows {}; using {}.", num_msaa_samples, driver_limit,
                    m_num_msaa_samples);
    }

    // Targets: force, torque and, for the pressure image, pressure (see pixel_loads_glsl.h).
    const size_t num_targets = m_keep_pressure_image ? 3 : 2;
    // The memory for num_pixel^2 * samples * targets * 16 B can still be too much even within
    // the driver's limit, so halve the sample count until the G-buffer can be allocated.
    while (!create_g_buffer(m_gpu->g_buffer_textures, m_gpu->g_buffer, num_pixel, m_num_msaa_samples, num_targets)) {
        if (m_num_msaa_samples == 1) {
            SPDLOG_ERROR("PixelForceTorqueCalculator: could not allocate the {}x{} G-buffer", num_pixel, num_pixel);
            throw std::runtime_error("PixelForceTorqueCalculator: could not allocate the G-buffer");
        }
        const unsigned int fewer = m_num_msaa_samples / 2;
        SPDLOG_WARN("Could not allocate the {}x{} G-buffer with {} samples; retrying with {}.", num_pixel, num_pixel,
                    m_num_msaa_samples, fewer);
        m_num_msaa_samples = fewer;
    }

    // With MSAA the samples are resolved in hardware (glBlitFramebuffer) into a single-sample
    // copy, which the integration reads: much faster than fetching every sample there.
    if (m_num_msaa_samples > 1 &&
        !create_g_buffer(m_gpu->resolved_textures, m_gpu->resolved, num_pixel, 1, num_targets)) {
        SPDLOG_ERROR("PixelForceTorqueCalculator: could not allocate the {}x{} resolved G-buffer", num_pixel,
                     num_pixel);
        throw std::runtime_error("PixelForceTorqueCalculator: could not allocate the resolved G-buffer");
    }

    // The model is evaluated once per pixel and triangle, with MSAA at the mean of the covered
    // samples (see g_buffer_fragment_shader).
    ShaderSource fragment(pixel_glsl::g_buffer_fragment_shader);
    fragment.set("num_msaa_samples", m_num_msaa_samples)
        .set("write_pressure", m_keep_pressure_image ? 1 : 0)
        .set("gsi_model", gsi_glsl);
    m_gpu->g_buffer_shader = std::make_unique<Shader>(ShaderSource(pixel_glsl::g_buffer_vertex_shader), fragment);
    if (m_num_msaa_samples > 1) {
        // The sample positions of the G-buffer, constant for its lifetime.
        std::vector<glm::vec2> offsets(m_num_msaa_samples);
        m_gpu->g_buffer->Bind();
        for (unsigned int s = 0; s < m_num_msaa_samples; ++s) {
            GLfloat position[2] = {0.5f, 0.5f};
            GLCall(glGetMultisamplefv(GL_SAMPLE_POSITION, s, position));
            offsets[s] = glm::vec2(position[0], position[1]) - 0.5f;
        }
        m_gpu->g_buffer->UnBind();
        m_gpu->g_buffer_shader->Bind();
        m_gpu->g_buffer_shader->SetUniform2fv("u_sample_offsets", offsets);
    }
    m_gpu->g_buffer_shader->Unbind();

    m_gpu->integrator = std::make_unique<PixelLoadIntegrator>(num_pixel, m_keep_pressure_image);

    // Raw geometry: the model matrices are applied on the GPU. Normals are per
    // triangle, so each one is repeated for the triangle's three de-indexed vertices.
    std::span<const float> vertices = m_geometry.get_untransformed_vertices();
    std::span<const float> normals = m_geometry.get_untransformed_normals();
    std::vector<float> vertex_normals(vertices.size());
    for (size_t triangle = 0; triangle < normals.size() / 3; ++triangle) {
        for (size_t corner = 0; corner < 3; ++corner) {
            for (size_t axis = 0; axis < 3; ++axis) {
                vertex_normals[9 * triangle + 3 * corner + axis] = normals[3 * triangle + axis];
            }
        }
    }

    m_gpu->vao = std::make_unique<VertexArray>();
    VertexBufferLayout layout_vertices;
    layout_vertices.Push<float>(3); // vec3 position
    VertexBuffer vb(vertices.data(), static_cast<unsigned int>(sizeof(float) * vertices.size()));
    m_gpu->vao->AddBuffer(vb, layout_vertices);

    VertexBufferLayout layout_normals;
    layout_normals.Push<float>(3); // vec3 normal
    VertexBuffer vb_normals(vertex_normals.data(), static_cast<unsigned int>(sizeof(float) * vertex_normals.size()));
    m_gpu->vao->AddBuffer(vb_normals, layout_normals);
    m_gpu->vao->Unbind();

    SPDLOG_DEBUG("PixelForceTorqueCalculator ready ({} triangles, {}x{} pixels)", m_geometry.get_num_triangles(),
                 num_pixel, num_pixel);
}

PixelForceTorqueCalculator::~PixelForceTorqueCalculator() = default;

int PixelForceTorqueCalculator::calc_aero_torque_force(const glm::vec3& v_rel__m_per_s, float surface_temp__K,
                                                       AeroConditions& aero, glm::vec3& torque__Nm,
                                                       glm::vec3& force__N) {
    torque__Nm = glm::vec3(0.0f);
    force__N = glm::vec3(0.0f);
    m_wetted_area__m2 = 0.0;
    m_projected_area__m2 = 0.0;

    const float rel_speed = glm::length(v_rel__m_per_s);
    if (!std::isfinite(rel_speed)) {
        SPDLOG_ERROR("calc_aero_torque_force called with a non-finite relative velocity ({}, {}, {}); the load is "
                     "undefined, returning NaN and an error code",
                     v_rel__m_per_s.x, v_rel__m_per_s.y, v_rel__m_per_s.z);
        torque__Nm = glm::vec3(std::numeric_limits<float>::quiet_NaN());
        force__N = glm::vec3(std::numeric_limits<float>::quiet_NaN());
        return -1;
    }
    if (rel_speed <= 0.0f) {
        SPDLOG_WARN("calc_aero_torque_force called with zero relative velocity; returning zero force/torque");
        return 0;
    }

    ScopedCurrentContext current(*m_gpu->context);

    // Pass 1: G-buffer of the front-most surface along the flow.
    const float bounding_sphere_radius = m_geometry.get_bounding_sphere_radius();
    const FlowCamera camera = make_flow_camera(glm::normalize(v_rel__m_per_s), bounding_sphere_radius);
    const glm::mat4 view_projection = camera.projection * camera.view;

    m_gpu->g_buffer->Bind();
    GLCall(glViewport(0, 0, static_cast<GLsizei>(m_num_pixel), static_cast<GLsizei>(m_num_pixel)));
    // Leeward faces are culled, as in the Binary and CoP shaders. On a two-sided plate
    // the leeward face lies exactly on the windward one; drawn, it could win the depth
    // tie and hide it. Closed or two-sided geometry needs no leeward face to shadow.
    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glCullFace(GL_BACK));
    GLCall(glFrontFace(GL_CCW));
    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glDepthFunc(GL_LESS));
    GLCall(glDepthMask(GL_TRUE));
    GLCall(glEnable(GL_MULTISAMPLE)); // no effect on a single-sample framebuffer
    m_gpu->g_buffer->Clear();

    m_gpu->g_buffer_shader->Bind();
    // The model is evaluated in this pass, see g_buffer_fragment_shader.
    Shader& shader = *m_gpu->g_buffer_shader;
    shader.setUniformMat4f("u_view_projection", view_projection);
    shader.SetUniform3f("u_v_rel", v_rel__m_per_s);
    shader.SetUniform1f("u_surface_temp", surface_temp__K);
    shader.SetUniform1f("u_min_cos_delta", m_min_cos_delta);
    shader.SetUniform1f("u_edge_on_threshold", edge_on_threshold(rel_speed));
    for (const GlslUniform& uniform : m_gsi_model.glsl_uniforms(aero)) {
        std::visit(
            [&](auto value) {
                if constexpr (std::is_same_v<decltype(value), int>) {
                    shader.SetUniform1i(uniform.name, value);
                } else {
                    shader.SetUniform1f(uniform.name, value);
                }
            },
            uniform.value);
    }
    m_gpu->vao->Bind();
    std::span<const glm::mat4> model_matrices = m_geometry.get_model_matrices();
    std::span<const unsigned int> num_triangles_per_mesh = m_geometry.get_num_triangles_per_mesh();
    unsigned int offset = 0;
    for (size_t i = 0; i < num_triangles_per_mesh.size(); i++) {
        const glm::mat4& model = model_matrices[i];
        // Same normal transform as RotatableMeshGeometry applies on the CPU.
        const glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
        m_gpu->g_buffer_shader->setUniformMat4f("u_model", model);
        m_gpu->g_buffer_shader->setUniformMat3f("u_normal_matrix", normal_matrix);
        GLCall(glDrawArrays(GL_TRIANGLES, offset, static_cast<GLsizei>(num_triangles_per_mesh[i] * 3)));
        offset += num_triangles_per_mesh[i] * 3;
    }
    m_gpu->vao->Unbind();
    m_gpu->g_buffer_shader->Unbind();
    m_gpu->g_buffer->UnBind();
    memory_barrier(Barrier::TextureFetch);

    // MSAA: average the samples of each pixel in hardware. Uncovered samples hold the clear
    // value 0, so each pixel gets the mean load of its samples and w of target 0 the coverage.
    if (m_gpu->resolved) {
        m_gpu->g_buffer->resolve_color_into(*m_gpu->resolved);
    }
    const auto& integrated_textures = m_gpu->resolved ? m_gpu->resolved_textures : m_gpu->g_buffer_textures;

    // Pass 2
    const float pixel_edge__m = 2.0f * bounding_sphere_radius / static_cast<float>(m_num_pixel);
    const float pixel_area__m2 = pixel_edge__m * pixel_edge__m;
    const PixelLoadIntegrator::Sums sums =
        m_gpu->integrator->integrate(integrated_textures, pixel_area__m2, m_pressure_image);

    glm::dvec3 force_sum__N = sums.force__N;
    glm::dvec3 torque_sum__Nm = sums.torque__Nm;
    m_wetted_area__m2 = sums.wetted_area__m2;
    m_projected_area__m2 = sums.windward_pixels * static_cast<double>(pixel_area__m2);

    // Leeward triangles are invisible to the camera: per triangle on the CPU, unshadowed.
    // Hyperthermal flow: leeward surfaces receive no load, so they are left out.
    if (m_flow_regime.load() == FlowRegime::Hypothermal) {
        std::span<const float> areas = m_geometry.get_areas();
        std::span<const float> normals = m_geometry.get_transformed_normals();
        std::span<const float> centroids = m_geometry.get_transformed_centroids();
        for (unsigned int i = 0; i < m_geometry.get_num_triangles(); i++) {
            const glm::vec3 normal{normals[3 * i], normals[3 * i + 1], normals[3 * i + 2]};
            // is_leeward is false for a NaN dot product, so degenerate triangles, which
            // have no area and would otherwise poison the sum, are skipped too.
            if (!is_leeward(glm::dot(normal, v_rel__m_per_s), rel_speed)) {
                continue;
            }
            const glm::vec3 centroid{centroids[3 * i], centroids[3 * i + 1], centroids[3 * i + 2]};
            glm::vec3 aero_force__N;
            glm::vec3 aero_torque__Nm;
            m_gsi_model.calc_aero_force_and_torque(areas[i], normal, centroid, v_rel__m_per_s, surface_temp__K, aero,
                                                   aero_force__N, aero_torque__Nm);
            force_sum__N += glm::dvec3(aero_force__N);
            torque_sum__Nm += glm::dvec3(aero_torque__Nm);
        }
    }

    force__N = glm::vec3(force_sum__N);
    torque__Nm = glm::vec3(torque_sum__Nm);

    SPDLOG_DEBUG("calc_aero_torque_force done (|F|={}, |T|={}, wetted area={} m^2)", glm::length(force__N),
                 glm::length(torque__Nm), m_wetted_area__m2);
    return 0;
}

} // namespace vat::loads
