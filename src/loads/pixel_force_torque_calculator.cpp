#include "pixel_force_torque_calculator.h"
#include <array>
#include <span>
#include <stdexcept>
#include <string>
#include <glm/glm.hpp>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

//custom abstractions
#include "flow_camera.h"
#include "frame_buffer.h"
#include "gl_helpers.h"
#include "glfw_opengl_context.h"
#include "scoped_context.h"
#include "shader.h"
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
    std::array<unsigned int, 2> g_buffer_textures{0, 0}; // position, normal
    std::unique_ptr<FrameBuffer> g_buffer;
    std::unique_ptr<Shader> g_buffer_shader;
    std::unique_ptr<VertexArray> vao;
    std::unique_ptr<PixelLoadIntegrator> integrator;

    ~GpuState() {
        if (!context) {
            return;
        }
        context->make_current();
        integrator.reset();
        vao.reset();
        g_buffer_shader.reset();
        g_buffer.reset();
        GLCall(glDeleteTextures(static_cast<GLsizei>(g_buffer_textures.size()), g_buffer_textures.data()));
    }
};

PixelForceTorqueCalculator::PixelForceTorqueCalculator(
    IGeometryShadingData& geometry,
    IGSIModel& gsi_model,
    unsigned int num_pixel,
    PixelLoadOptions options)
    : m_geometry(geometry),
      m_gsi_model(gsi_model),
      m_num_pixel(num_pixel),
      m_options(options),
      m_gpu(std::make_unique<GpuState>()) {
    const std::string gsi_glsl = m_gsi_model.glsl_force_per_projected_area();
    if (gsi_glsl.empty()) {
        SPDLOG_ERROR("PixelForceTorqueCalculator: the GSI model has no GPU implementation");
        throw std::invalid_argument("PixelForceTorqueCalculator: the GSI model has no GPU implementation");
    }
    if (num_pixel == 0) {
        SPDLOG_ERROR("PixelForceTorqueCalculator: num_pixel must be positive");
        throw std::invalid_argument("PixelForceTorqueCalculator: num_pixel must be positive");
    }

    // The window is never shown and never drawn to; all rendering goes to the G-buffer.
    m_gpu->context = std::make_unique<GlfwOpenGLContext>(64, 64, "Pixel Load Renderer", false);
    ScopedCurrentContext current(*m_gpu->context);

    // G-buffer: two float targets, both sampled texel by texel, never filtered.
    GLCall(glGenTextures(static_cast<GLsizei>(m_gpu->g_buffer_textures.size()), m_gpu->g_buffer_textures.data()));
    for (unsigned int texture : m_gpu->g_buffer_textures) {
        GLCall(glBindTexture(GL_TEXTURE_2D, texture));
        GLCall(glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, num_pixel, num_pixel));
        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    }
    GLCall(glBindTexture(GL_TEXTURE_2D, 0));
    m_gpu->g_buffer = std::make_unique<FrameBuffer>(std::span<const unsigned int>(m_gpu->g_buffer_textures), num_pixel, num_pixel);
    m_gpu->g_buffer->UnBind();

    m_gpu->g_buffer_shader = std::make_unique<Shader>(pixel_glsl::g_buffer_vertex_shader, pixel_glsl::g_buffer_fragment_shader, true);
    m_gpu->g_buffer_shader->Unbind();

    m_gpu->integrator = std::make_unique<PixelLoadIntegrator>(gsi_glsl, num_pixel, m_options.keep_pressure_image);

    // Raw geometry: the model matrices are applied on the GPU. Normals are per
    // triangle, so each one is repeated for the triangle's three de-indexed vertices.
    std::span<const float> vertices = m_geometry.get_raw_vertices();
    std::span<const float> normals = m_geometry.get_raw_normals();
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

    SPDLOG_DEBUG("PixelForceTorqueCalculator ready ({} triangles, {}x{} pixels)",
        m_geometry.get_num_triangles(), num_pixel, num_pixel);
}

PixelForceTorqueCalculator::~PixelForceTorqueCalculator() = default;

int PixelForceTorqueCalculator::calc_aero_torque_force(const glm::vec3& v_rel__m_per_s, float surface_temp__K, AeroConditions& aero, glm::vec3& torque__Nm, glm::vec3& force__N) {
    torque__Nm = glm::vec3(0.0f);
    force__N = glm::vec3(0.0f);
    m_wetted_area__m2 = 0.0;
    m_projected_area__m2 = 0.0;

    const float rel_speed = glm::length(v_rel__m_per_s);
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
    // No face culling: a surface facing away from the flow still shadows what lies
    // behind it. Its own pixels carry no load; the integration pass skips them.
    GLCall(glDisable(GL_CULL_FACE));
    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glDepthFunc(GL_LESS));
    GLCall(glDepthMask(GL_TRUE));
    m_gpu->g_buffer->ClearFloat();

    m_gpu->g_buffer_shader->Bind();
    m_gpu->g_buffer_shader->setUniformMat4f("u_view_projection", view_projection);
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
    GLCall(glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT));

    // Pass 2: GSI per windward pixel, reduced on the GPU.
    const float pixel_edge__m = 2.0f * bounding_sphere_radius / static_cast<float>(m_num_pixel);
    const float pixel_area__m2 = pixel_edge__m * pixel_edge__m;
    const PixelLoadIntegrator::Sums sums = m_gpu->integrator->integrate(
        m_gpu->g_buffer_textures[0], m_gpu->g_buffer_textures[1],
        v_rel__m_per_s, surface_temp__K, pixel_area__m2, m_options.min_cos_delta,
        m_gsi_model.glsl_uniforms(aero), m_pressure_image);

    glm::dvec3 force_sum__N = sums.force__N;
    glm::dvec3 torque_sum__Nm = sums.torque__Nm;
    m_wetted_area__m2 = sums.wetted_area__m2;
    m_projected_area__m2 = sums.windward_pixels * static_cast<double>(pixel_area__m2);

    // Leeward triangles are invisible to the camera: per triangle on the CPU, unshadowed.
    std::span<const float> areas = m_geometry.get_areas();
    std::span<const float> normals = m_geometry.get_normals();
    std::span<const float> centroids = m_geometry.get_centroids();
    for (unsigned int i = 0; i < m_geometry.get_num_triangles(); i++) {
        const glm::vec3 normal{normals[3 * i], normals[3 * i + 1], normals[3 * i + 2]};
        // Written as !(... <= 0) so degenerate triangles, whose normal is NaN, are
        // skipped too: they have no area and would otherwise poison the sum.
        if (!(glm::dot(normal, v_rel__m_per_s) <= 0.0f)) {
            continue;
        }
        const glm::vec3 centroid{centroids[3 * i], centroids[3 * i + 1], centroids[3 * i + 2]};
        glm::vec3 aero_force__N;
        glm::vec3 aero_torque__Nm;
        m_gsi_model.calc_aero_force_and_torque(areas[i], normal, centroid, v_rel__m_per_s, surface_temp__K, aero, aero_force__N, aero_torque__Nm);
        force_sum__N += glm::dvec3(aero_force__N);
        torque_sum__Nm += glm::dvec3(aero_torque__Nm);
    }

    force__N = glm::vec3(force_sum__N);
    torque__Nm = glm::vec3(torque_sum__Nm);

    SPDLOG_DEBUG("calc_aero_torque_force done (|F|={}, |T|={}, wetted area={} m^2)",
        glm::length(force__N), glm::length(torque__Nm), m_wetted_area__m2);
    return 0;
}

} // namespace vat::loads
