#include "gpu_aero_load_calculator.h"
#include "frame_buffer.h"
#include "texture_2d.h"
#include "vertex_buffer.h"
#include "vertex_buffer_layout.h"
#include "shaders/vertex_frag_shader.h"
#include "shaders/computer_force_shader.h"
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


GPUAeroLoadCalculator::GPUAeroLoadCalculator(ISatelliteShadingData& satellite, int num_pixel)
    :m_satellite(satellite), m_num_pixel(num_pixel) {

    m_context = std::make_unique<GlfwOpenGLContext>(num_pixel, num_pixel, "GPU Aero Load Calculator", false);
    m_context->make_current();
    
    // Create shader program from embedded sources
    m_shader = std::make_unique<Shader>(ID_vertex_shader, ID_fragment_shader, true);
    m_shader->unbind();
    m_compute_shader = std::make_unique<ComputeShader>(Compute_shader, true);
    m_compute_shader->unbind();

    // Enable depth testing for proper occlusion
    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glDepthFunc(GL_LESS));
    GLCall(glDepthMask(GL_TRUE));

    // Enable face culling
    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glCullFace(GL_BACK));
    GLCall(glFrontFace(GL_CCW)); // Counter-clockwise is front-facing

    //create framebuffers
    m_position_texture = std::make_unique<Texture2D>(m_num_pixel, m_num_pixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    m_normal_texture = std::make_unique<Texture2D>(m_num_pixel, m_num_pixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    m_float_texture = std::make_unique<Texture2D>(m_num_pixel, m_num_pixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);

    m_frame_buffer = std::make_unique<FrameBuffer>(m_num_pixel, m_num_pixel, m_position_texture.get());
    m_frame_buffer->attach_texture_2d(m_normal_texture.get());
    m_frame_buffer->attach_texture_2d(m_float_texture.get());

    // 5. Specify which color attachments the shader will write into

    m_frame_buffer->bind();
    GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    GLCall(glDrawBuffers(3, attachments));
    const GLenum framebuffer_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (framebuffer_status != GL_FRAMEBUFFER_COMPLETE) {
        SPDLOG_ERROR("GPU framebuffer is incomplete after attaching textures (status={})", framebuffer_status);
    }
    m_frame_buffer->unbind();

    std::span<const float> vertices = m_satellite.get_vertices();
    std::span<const float> normals = m_satellite.get_normals();

    std::vector<float> vertex_normals;
    vertex_normals.reserve(static_cast<std::size_t>(m_satellite.get_num_triangles()) * 9u);
    for (unsigned int triangle_idx = 0; triangle_idx < m_satellite.get_num_triangles(); ++triangle_idx) {
        const std::size_t normal_base = static_cast<std::size_t>(triangle_idx) * 3u;
        const float nx = normals[normal_base];
        const float ny = normals[normal_base + 1u];
        const float nz = normals[normal_base + 2u];
        vertex_normals.insert(vertex_normals.end(), { nx, ny, nz, nx, ny, nz, nx, ny, nz });
    }

    //print the first 10 normals for debugging
    for (int i = 0; i < std::min(10, static_cast<int>(vertex_normals.size() / 3)); ++i) {
        SPDLOG_INFO("Vertex normal {}: ({}, {}, {})", i, vertex_normals[i * 3], vertex_normals[i * 3 + 1], vertex_normals[i * 3 + 2]);
    }

	m_vertex_array.reset(new VertexArray());
    VertexBufferLayout layoutVertices;
    layoutVertices.push<float>(3);           // vec3 position
    VertexBuffer vb(vertices.data(), static_cast<unsigned int>(sizeof(float) * vertices.size()));
    m_vertex_array->add_buffer(vb, layoutVertices);

    VertexBufferLayout layoutNormals;
    layoutNormals.push<float>(3);           // vec3 normal
    VertexBuffer vbNormals(vertex_normals.data(), static_cast<unsigned int>(sizeof(float) * vertex_normals.size()));
    m_vertex_array->add_buffer(vbNormals, layoutNormals);
}

int GPUAeroLoadCalculator::calc_aero_torque_force(const glm::vec3 &v_rel__m_per_s, float surface_temp__K, AeroConditions &aero, glm::vec3 &torque__Nm, glm::vec3 &force__N) {
    m_context->make_current();
    //projection matrices
    glm::vec3 v_rel_hat = normalize(v_rel__m_per_s);
    float bounding_sphere_radius = m_satellite.get_bounding_sphere_radius();
    float pixel_length = 2.0f * bounding_sphere_radius / static_cast<float>(m_num_pixel);
    float pixel_area = pixel_length * pixel_length;
    SPDLOG_TRACE("pixel_area = {}", pixel_area);
    glm::vec3 camera_position = v_rel_hat * bounding_sphere_radius;

    glm::mat4 orthoProj = glm::ortho(-bounding_sphere_radius,
        bounding_sphere_radius,
        -bounding_sphere_radius,
        bounding_sphere_radius,
        0.0f,
        2 * bounding_sphere_radius
    );

    glm::vec3 target = glm::vec3(0.0f);
    glm::vec3 forward = glm::normalize(target - camera_position);

    glm::vec3 ref = (std::abs(forward.y) < 0.99f)
        ? glm::vec3(0.0f, 1.0f, 0.0f)
        : glm::vec3(1.0f, 0.0f, 0.0f);

    glm::vec3 right = glm::normalize(glm::cross(forward, ref));
    glm::vec3 up    = glm::normalize(glm::cross(right, forward));

    glm::mat4 view = glm::lookAt(
        camera_position,
        target,
        up
    );

    m_frame_buffer->bind();
    m_frame_buffer->clear();
    GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    GLCall(glDrawBuffers(3, attachments));

	//render triangles and cops to Framebuffer
    m_shader->bind();
	m_vertex_array->bind();
    std::span<const unsigned int> num_triangles_per_mesh = m_satellite.get_num_triangles_per_mesh();
    std::span<const glm::mat4> model_matrices = m_satellite.get_model_matrices();
	unsigned int triangle_offset = 0;
    for (int i = 0; i < num_triangles_per_mesh.size(); i++) {
		glm::mat4 model = model_matrices[i];
        glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
        //print nomral matrix to debugging
        SPDLOG_TRACE("Normal matrix for mesh {}: ({}, {}, {})", i, normal_matrix[0][0], normal_matrix[0][1], normal_matrix[0][2]);
        SPDLOG_TRACE("Normal matrix for mesh {}: ({}, {}, {})", i, normal_matrix[1][0], normal_matrix[1][1], normal_matrix[1][2]);
        SPDLOG_TRACE("Normal matrix for mesh {}: ({}, {}, {})", i, normal_matrix[2][0], normal_matrix[2][1], normal_matrix[2][2]);
        m_shader->set_uniform_mat4f("model",model);
        m_shader->set_uniform_mat4f("view",view);
        m_shader->set_uniform_mat4f("projection",orthoProj);
        m_shader->set_uniform_mat3f("normalMatrix",normal_matrix);
        m_shader->set_uniform_3f("windDir",v_rel_hat);
        glDrawArrays(GL_TRIANGLES, triangle_offset, static_cast<GLsizei>(num_triangles_per_mesh[i] * 3));
        triangle_offset += num_triangles_per_mesh[i] * 3;
    }
    m_vertex_array->unbind();
    m_shader->unbind();

    m_normal_texture->plot_texture("Normal Texture");
    m_float_texture->plot_texture("Float Texture");
    m_position_texture->plot_texture("Position Texture");

    SPDLOG_INFO("Dispatching compute shader with {}x{} groups.", (m_num_pixel + 15u) / 16u, (m_num_pixel + 15u) / 16u);
    const GLuint groups_x = (m_num_pixel + 15u) / 16u;
    const GLuint groups_y = (m_num_pixel + 15u) / 16u;

    struct ForceTorqueData {
        glm::ivec3 force;
        glm::ivec3 torque;
    };

    ForceTorqueData force_torque_data{ glm::ivec3(0), glm::ivec3(0) };
    GLuint force_torque_buffer = 0;
    GLCall(glGenBuffers(1, &force_torque_buffer));
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, force_torque_buffer));
    GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ForceTorqueData), &force_torque_data, GL_DYNAMIC_DRAW));
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));

    m_compute_shader->bind();
    m_compute_shader->set_uniform_1f("pixelArea", pixel_area);
    m_compute_shader->set_uniform_1f("density", aero.density__kg_per_m3);
    m_compute_shader->set_uniform_1f("velocity_mag", glm::length(v_rel__m_per_s));
    GLCall(glBindImageTexture(0, m_position_texture->get_texture_id(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F));
    GLCall(glBindImageTexture(1, m_normal_texture->get_texture_id(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F));
    GLCall(glBindImageTexture(2, m_float_texture->get_texture_id(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F));
    GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, force_torque_buffer));
    GLCall(glDispatchCompute(groups_x, groups_y, 1u));
    GLCall(glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT));
    m_compute_shader->unbind();

    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, force_torque_buffer));
    GLCall(glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ForceTorqueData), &force_torque_data));
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
    GLCall(glDeleteBuffers(1, &force_torque_buffer));

    force__N = glm::vec1(1.0e-9)* glm::vec3(force_torque_data.force);
    torque__Nm =  glm::vec1(1.0e-9)* glm::vec3(force_torque_data.torque);
    return 0;
}
