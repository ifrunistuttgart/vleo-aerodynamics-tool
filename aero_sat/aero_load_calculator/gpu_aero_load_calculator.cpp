#include "gpu_aero_load_calculator.h"
#include "frame_buffer.h"
#include "texture_2d.h"
#include "vertex_buffer.h"
#include "vertex_buffer_layout.h"
#include "shaders/fragment_shader.h"
#include "shaders/compute_aggregate_force_shader.h"
#include "shaders/double_pass_compute_shader.h"
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <numbers>

GPUAeroLoadCalculator::GPUAeroLoadCalculator(ISatelliteShadingData& satellite, IGSIModelGPU& gsi_model, int num_pixel)
    :m_satellite(satellite),m_gsi_model(gsi_model), m_num_pixel(num_pixel),m_groups((m_num_pixel + 15u) / 16u) {

    // Check if float atomics are supported
    // After initializing your GLFW window and loading OpenGL pointers
    bool hasFloatAtomics = false;

    #ifdef GL_VERSION_3_0
        // Method 1: Query by extension count
        GLint numExtensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
        for (int i = 0; i < numExtensions; i++) {
            std::string ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
            if (ext == "GL_EXT_shader_atomic_float") {
                hasFloatAtomics = true;
                SPDLOG_INFO("Float atomics are supported!");
            }
            if (ext == "GL_ARB_gpu_shader_int64") {
                SPDLOG_INFO("64-bit integer arithmetics are supported!");
            }
            if (ext == "GL_EXT_shader_atomic_int64") {
                SPDLOG_INFO("64-bit integer atomics are supported!");
            }
        }
    #else
        // Method 2: Legacy fallback string query
        const char* extString = (const char*)glGetString(GL_EXTENSIONS);
        if (extString && strstr(extString, "GL_EXT_shader_atomic_float")) {
            hasFloatAtomics = true;
            SPDLOG_INFO("Float atomics are supported!");
        }
    #endif


    m_context = std::make_unique<GlfwOpenGLContext>(num_pixel, num_pixel, "GPU Aero Load Calculator", false);
    m_context->make_current();
    
    // Create shader program from embedded sources
    m_shader = std::make_unique<Shader>(m_gsi_model.get_vertex_shader_code(), gsi_fragment_shader, true);
    m_shader->unbind();
    m_compute_shader = std::make_unique<ComputeShader>(compute_shader1, true);
    m_compute_shader->unbind();

    m_compute_shader2 = std::make_unique<ComputeShader>(compute_shader2, true);
    m_compute_shader2->unbind();

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
    m_pressure_vec_texture = std::make_unique<Texture2D>(m_num_pixel, m_num_pixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    m_float_texture = std::make_unique<Texture2D>(m_num_pixel, m_num_pixel, GL_RGBA32F, GL_RGBA, GL_FLOAT);

    m_frame_buffer = std::make_unique<FrameBuffer>(m_num_pixel, m_num_pixel, m_position_texture.get());
    m_frame_buffer->attach_texture_2d(m_pressure_vec_texture.get());
    m_frame_buffer->attach_texture_2d(m_float_texture.get());
    m_frame_buffer->unbind();

    std::span<const float> vertices = m_satellite.get_raw_vertices();
    std::span<const float> normals = m_satellite.get_raw_normals();

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
        SPDLOG_TRACE("Vertex normal {}: ({}, {}, {})", i, vertex_normals[i * 3], vertex_normals[i * 3 + 1], vertex_normals[i * 3 + 2]);
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

    m_intermediate_ssbo = std::make_unique<ShaderStorageBuffer>(nullptr, sizeof(double)*8 * m_groups*m_groups);
    m_ssbo = std::make_unique<ShaderStorageBuffer>(&m_force_torque_data, sizeof(ForceTorqueData));
}

GPUAeroLoadCalculator::~GPUAeroLoadCalculator() {
    m_context->make_current();
    m_shader.reset();
    m_compute_shader.reset();
    m_frame_buffer.reset();
    m_vertex_array.reset();
    m_position_texture.reset();
    m_pressure_vec_texture.reset();
    m_float_texture.reset();
    m_ssbo.reset();
    m_intermediate_ssbo.reset();
    m_compute_shader2.reset();
    m_context.reset();

}

int GPUAeroLoadCalculator::calc_aero_torque_force(const glm::vec3 &v_rel__m_per_s, float surface_temp__K, AeroConditions &aero, glm::vec3 &torque__Nm, glm::vec3 &force__N) {
    m_context->make_current();

    //projection matrices
    glm::vec3 v_rel_hat = normalize(v_rel__m_per_s);
    float bounding_sphere_radius = m_satellite.get_bounding_sphere_radius();
    float pixel_length = 2.0f * bounding_sphere_radius / static_cast<float>(m_num_pixel);
    float pixel_area = pixel_length * pixel_length;
    float aero_pressure = 0.5f * aero.density__kg_per_m3 * glm::length(v_rel__m_per_s) * glm::length(v_rel__m_per_s);
    float max_possible_force = aero_pressure * 2.5f * bounding_sphere_radius * bounding_sphere_radius * std::numbers::pi;
    int exponent = static_cast<int>(std::floor(std::log10(std::abs(max_possible_force))));
    int max_possible_exponent = 9 - exponent -1;
    SPDLOG_INFO("Max possible exponent: {}", max_possible_exponent);
    SPDLOG_INFO("pixelArea: {}", pixel_area);
    SPDLOG_INFO("Aero pressure: {}", aero_pressure);
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
    // 1. Setup Query Object
    GLuint sampleQuery;
    glGenQueries(1, &sampleQuery);
    // Begin counting fragments
    glBeginQuery(GL_SAMPLES_PASSED, sampleQuery);
    GLCall(glEnable(GL_DEPTH_CLAMP));
    m_frame_buffer->bind();
    m_frame_buffer->clear();

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
        m_shader->set_uniform_1f("aero_pressure", aero_pressure);
        m_gsi_model.set_shader_uniforms(m_shader.get());
        glDrawArrays(GL_TRIANGLES, triangle_offset, static_cast<GLsizei>(num_triangles_per_mesh[i] * 3));
        triangle_offset += num_triangles_per_mesh[i] * 3;
    }
    // End counting
    glEndQuery(GL_SAMPLES_PASSED);
    // --- RENDER PASS END ---

    // 2. Retrieve Results (Retrieval blocks until GPU finishes the draw call)
    GLuint64 filledPixels = 0;
    glGetQueryObjectui64v(sampleQuery, GL_QUERY_RESULT, &filledPixels);
    double gpuProjectedArea = static_cast<double>(filledPixels) * pixel_area;
    SPDLOG_INFO("GPU projected area: {}", gpuProjectedArea);
    m_vertex_array->unbind();
    m_shader->unbind();
    m_frame_buffer->unbind();

    GLCall(glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT));

    m_ssbo->set_zero();
    m_intermediate_ssbo->set_zero();
    m_compute_shader->bind();
    m_compute_shader->set_uniform_1f("pixelArea", pixel_area);
    m_compute_shader->set_texture(0, *m_position_texture);
    m_compute_shader->set_texture(1, *m_pressure_vec_texture);
    m_compute_shader->set_texture(2, *m_float_texture);
    m_ssbo->bind_base(4);
    m_intermediate_ssbo->bind_base(3);
    m_compute_shader->run(m_groups, m_groups, 1);
    m_compute_shader->unbind();

    m_compute_shader2->bind();
    m_compute_shader2->set_uniform_1ui("numWorkGroupsToAggregate", m_groups*m_groups);
    m_compute_shader2->run(1,1,1);
    m_compute_shader2->unbind();
    m_ssbo->get_data(&m_force_torque_data, sizeof(ForceTorqueData));

    force__N = glm::vec3(m_force_torque_data.force);
    torque__Nm =  glm::vec3(m_force_torque_data.torque);
    return 0;
}
