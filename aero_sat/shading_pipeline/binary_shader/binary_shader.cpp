#include "binary_shader.h"

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//custom abstractions
#include "opengl/vertex_buffer.h"
#include "opengl/gl_helpers.h"

// embedded shader headers
#include "binary_shader/shaders/id_shader.h"
#include "opengl/vertex_buffer_layout.h"

BinaryShader::BinaryShader(unsigned int num_pixel)
    : NUM_PIXEL(num_pixel)
{

}

BinaryShader::~BinaryShader() {
    m_shader.reset();
    m_frame_buffer.reset();
    m_vao.reset();

    if (m_ID_texture != 0) {
        GLCall(glDeleteTextures(1, &m_ID_texture));
    }
}

int BinaryShader::set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) {
	m_numTriangles = static_cast<unsigned int>(triangleIDs.size() / 3); //assume 3 vertices per triangle
    if (m_numTriangles > MAX_TRIANGLES) {
		SPDLOG_ERROR("Number of triangles ({}) exceeds the maximum supported ({}).", m_numTriangles, MAX_TRIANGLES);
        return -1;
	}
    // framebuffer for counting ids
    SPDLOG_DEBUG("create framebuffer with ID texture of size {}x{}", NUM_PIXEL, NUM_PIXEL);
    GLCall(glGenTextures(1, &m_ID_texture));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_ID_texture));
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, NUM_PIXEL, NUM_PIXEL, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr));

    m_frame_buffer.reset(new FrameBuffer(m_ID_texture, NUM_PIXEL, NUM_PIXEL));
    m_frame_buffer->UnBind();

    // Create shader program from embedded sources
    m_shader.reset(new Shader(ID_vertex_shader, ID_fragment_shader, true));
    m_shader->Unbind();

    // Enable depth testing for proper occlusion
    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glDepthFunc(GL_LESS));
    GLCall(glDepthMask(GL_TRUE));

    // Enable face culling
    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glCullFace(GL_BACK));
    GLCall(glFrontFace(GL_CCW)); // Counter-clockwise is front-facing

    m_lenVertices = vertices.size();
	m_vao.reset(new VertexArray());
    VertexBufferLayout layoutVertices;
    layoutVertices.Push<float>(3);           // vec3 position
    VertexBuffer vb(vertices.data(), static_cast<unsigned int>(sizeof(float) * vertices.size()));
    m_vao->AddBuffer(vb, layoutVertices);

    VertexBufferLayout layoutIDs;
    layoutIDs.Push<unsigned int>(1);         // triangle ID
    VertexBuffer vbID(triangleIDs.data(), static_cast<unsigned int>(sizeof(std::uint32_t) * triangleIDs.size()));
    m_vao->AddBuffer(vbID, layoutIDs);
    return 0;
}

std::vector<float> BinaryShader::shade_satellite(glm::vec3 v_rel_hat, float bounding_sphere_radius, std::span<const unsigned int> num_triangles_per_mesh, std::span<const glm::mat4> model_matrices) {
    std::vector<float> triangle_visibility(m_numTriangles,0);

    //projection matrices
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

    m_frame_buffer->Bind();
    m_frame_buffer->Clear();

	//render to framebuffer with ID shader
    m_shader->Bind();
	m_vao->Bind();
	unsigned int offset = 0;
    for (int i = 0; i < num_triangles_per_mesh.size(); i++) {
		glm::mat4 model = model_matrices[i];
        glm::mat4 u_MVP = orthoProj * view * model;

        m_shader->setUniformMat4f("u_MVP", u_MVP);
        glDrawArrays(GL_TRIANGLES, offset, static_cast<GLsizei>(num_triangles_per_mesh[i] * 3));
        offset += num_triangles_per_mesh[i] * 3;
    }

	// Ensure all framebuffer writes are visible to the subsequent imageLoad in the compute shader.
    // GL_FRAMEBUFFER_BARRIER_BIT only covers framebuffer->framebuffer visibility.
    // GL_SHADER_IMAGE_ACCESS_BARRIER_BIT is required for framebuffer writes to be visible
    // to imageLoad in compute shaders. NVIDIA flushes all caches on any barrier (masking
    // the bug); AMD/Intel are spec-precise and only flush the framebuffer cache.
    // Read pixel IDs back to CPU via glReadPixels.
    // imageLoad from uimage2D is unreliable on AMD/Intel (returns 0 for all pixels);
    // glReadPixels from the bound FBO is spec-guaranteed and works on all drivers.
    GLCall(glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT));
    GLCall(glReadBuffer(GL_COLOR_ATTACHMENT0));
    std::vector<GLuint> pixel_ids(NUM_PIXEL * NUM_PIXEL, 0);
    GLCall(glReadPixels(0, 0, NUM_PIXEL, NUM_PIXEL, GL_RED_INTEGER, GL_UNSIGNED_INT, pixel_ids.data()));
    m_frame_buffer->UnBind();

    // Count visible triangles on CPU
    const size_t count = std::min(triangle_visibility.size(), static_cast<size_t>(m_numTriangles));
    std::vector<bool> seen(m_numTriangles + 1, false);
    for (GLuint id : pixel_ids) {
        if (id > 0 && id <= m_numTriangles) seen[id] = true;
    }
    int visible = 0;
    for (size_t i = 0; i < count; i++) {
        if (seen[i + 1]) { triangle_visibility[i] = 1.0f; visible++; }
    }
    SPDLOG_INFO("BinaryShader: {}/{} panels visible", visible, m_numTriangles);

	m_vao->Unbind();
    return triangle_visibility;
};