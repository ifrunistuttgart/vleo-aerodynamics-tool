#include "binary_shader.h"

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

//custom abstractions
#include "vertex_buffer.h"
#include "gl_helpers.h"

// embedded shader headers
#include "binary_shader/shaders/id_shader.h"
#include "vertex_buffer_layout.h"

BinaryShader::BinaryShader(unsigned int num_pixel)
    : NUM_PIXEL(num_pixel)
{
    // framebuffer for counting ids
    SPDLOG_DEBUG("create framebuffer with ID texture of size {}x{}", NUM_PIXEL, NUM_PIXEL);
    m_texture = std::make_unique<Texture2D>(num_pixel, num_pixel, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT);

    m_frame_buffer = std::make_unique<FrameBuffer>(NUM_PIXEL, NUM_PIXEL,m_texture.get());
    m_frame_buffer->unbind();

    m_visibility_reducer = std::make_unique<VisibilityReducer>(m_numTriangles);

    // Create shader program from embedded sources
    m_shader = std::make_unique<Shader>(ID_vertex_shader, ID_fragment_shader, true);
    m_shader->unbind();

    // Enable depth testing for proper occlusion
    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glDepthFunc(GL_LESS));
    GLCall(glDepthMask(GL_TRUE));

    // Enable face culling
    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glCullFace(GL_BACK));
    GLCall(glFrontFace(GL_CCW)); // Counter-clockwise is front-facing

}

BinaryShader::~BinaryShader() {
    m_shader.reset();
    m_visibility_reducer.reset();
    m_frame_buffer.reset();
    m_vao.reset();
    m_texture.reset();
}

int BinaryShader::set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) {
	m_numTriangles = static_cast<unsigned int>(triangleIDs.size() / 3); //assume 3 vertices per triangle
    if (m_numTriangles > MAX_TRIANGLES) {
		SPDLOG_ERROR("Number of triangles ({}) exceeds the maximum supported ({}).", m_numTriangles, MAX_TRIANGLES);
        return -1;
	}

    m_lenVertices = vertices.size();
	m_vao = std::make_unique<VertexArray>();
    VertexBufferLayout layoutVertices;
    layoutVertices.push<float>(3);           // vec3 position
    VertexBuffer vb(vertices.data(), static_cast<unsigned int>(sizeof(float) * vertices.size()));
    m_vao->add_buffer(vb, layoutVertices);

    VertexBufferLayout layoutIDs;
    layoutIDs.push<unsigned int>(1);         // triangle ID
    VertexBuffer vbID(triangleIDs.data(), static_cast<unsigned int>(sizeof(std::uint32_t) * triangleIDs.size()));
    m_vao->add_buffer(vbID, layoutIDs);
    return 0;
}

std::vector<float> BinaryShader::shade_satellite(glm::vec3 v_rel_hat, float bounding_sphere_radius, std::span<const unsigned int> num_triangles_per_mesh, std::span<const glm::mat4> model_matrices) {
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

    m_frame_buffer->bind();
    m_frame_buffer->clear();

	//render to framebuffer with ID shader
    m_shader->bind();
	m_vao->bind();
	unsigned int offset = 0;
    for (int i = 0; i < num_triangles_per_mesh.size(); i++) {
		glm::mat4 model = model_matrices[i];
        glm::mat4 u_MVP = orthoProj * view * model;

        m_shader->set_uniform_mat4f("u_MVP", u_MVP);
        glDrawArrays(GL_TRIANGLES, offset, static_cast<GLsizei>(num_triangles_per_mesh[i] * 3));
        offset += num_triangles_per_mesh[i] * 3;
    }
    m_vao->unbind();
    m_shader->unbind();

    // Make the rendered IDs visible to the reduction shader, then reduce on the GPU:
    // only one flag per triangle is read back, not the whole NUM_PIXEL^2 image.
    GLCall(glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT));
    m_frame_buffer->unbind();
    return m_visibility_reducer->reduce(m_texture.get());
};