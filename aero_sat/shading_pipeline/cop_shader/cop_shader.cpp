#include "cop_shader.h"
#define FMT_UNICODE 0 // avoid error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//custom abstractions
#include "vertex_buffer.h"
#include "gl_helpers.h"
#include "vertex_buffer_layout.h"

// embedded shader headers
#include "cop_shader/shaders/id_shader.h"


CoPShader::CoPShader(unsigned int num_pixel)
    : NUM_PIXEL(num_pixel) {

    // Create shader program from embedded sources
    m_shader.reset(new Shader(ID_vertex_shader, ID_fragment_shader, true));
    m_shader->unbind();

    m_point_shader.reset(new Shader(ID_point_shader, ID_fragment_shader, true));
    m_point_shader->unbind();

    // Enable depth testing for proper occlusion
    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glDepthFunc(GL_LESS));
    GLCall(glDepthMask(GL_TRUE));

    // Enable face culling
    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glCullFace(GL_BACK));
    GLCall(glFrontFace(GL_CCW)); // Counter-clockwise is front-facing

    // framebuffer for counting ids
    SPDLOG_DEBUG("create framebuffer with ID texture of size {}x{}", NUM_PIXEL, NUM_PIXEL);
    m_texture.reset(new Texture2D(num_pixel,num_pixel, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT));

    m_frame_buffer.reset(new FrameBuffer(NUM_PIXEL, NUM_PIXEL, m_texture.get()));
    m_frame_buffer->unbind();
};

CoPShader::~CoPShader() {
    m_shader.reset();
    m_point_shader.reset();
    m_frame_buffer.reset();
    m_triangle_vao.reset();
    m_cop_vao.reset();
    m_texture.reset();
}

int CoPShader::set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) {
    m_numTriangles = static_cast<unsigned int>(triangleIDs.size() / 3); //assume 3 vertices per triangle
    if (m_numTriangles > MAX_TRIANGLES) {
		SPDLOG_ERROR("Number of triangles ({}) exceeds the maximum supported ({}).", m_numTriangles, MAX_TRIANGLES);
        return -1;
	}

    m_visibility_reducer = std::make_unique<VisibilityReducer>(m_numTriangles);

    m_lenVertices = vertices.size();
	m_triangle_vao.reset(new VertexArray());
    VertexBufferLayout layoutVertices;
    layoutVertices.push<float>(3);           // vec3 position
    VertexBuffer vb(vertices.data(), static_cast<unsigned int>(sizeof(float) * vertices.size()));
    m_triangle_vao->add_buffer(vb, layoutVertices);

    //set triangle IDs to background IDs
    VertexBufferLayout layout_background_ids;
    layout_background_ids.push<unsigned int>(1);         // triangle ID
    std::vector<std::uint32_t> background_id(triangleIDs.size(),0);
    VertexBuffer vb_triangle_ID(background_id.data(), static_cast<unsigned int>(sizeof(std::uint32_t) * background_id.size()));
    m_triangle_vao->add_buffer(vb_triangle_ID, layout_background_ids);

    //compute triangle centroids from vertices
    std::vector<float> cop(m_numTriangles*3,0);
    for (unsigned int i = 0; i < m_numTriangles; i++) {
        cop[i*3] = (vertices[i*9] + vertices[i*9+3] + vertices[i*9+6]) / 3.0f;
        cop[i*3+1] = (vertices[i*9+1] + vertices[i*9+4] + vertices[i*9+7]) / 3.0f;
        cop[i*3+2] = (vertices[i*9+2] + vertices[i*9+5] + vertices[i*9+8]) / 3.0f;
    }

    m_cop_vao.reset(new VertexArray());
    VertexBufferLayout layoutCop;
    layoutCop.push<float>(3);
    VertexBuffer vbCop(cop.data(), static_cast<unsigned int>(sizeof(float) * cop.size()));
    m_cop_vao->add_buffer(vbCop, layoutCop);

    //set triangle IDs for Cops
    VertexBufferLayout layout_cop_ids;
    layout_cop_ids.push<unsigned int>(1);         // triangle ID
    //since trianlgeIds has one value for each vertex cop ids should take every 3rd entry from triangleIds
    std::vector<std::uint32_t> cop_triangle_ids(m_numTriangles,0);
    for (unsigned int i = 0; i < m_numTriangles; i++) {
        cop_triangle_ids[i] = triangleIDs[i*3];
    }
    VertexBuffer vb_cop_ID(cop_triangle_ids.data(), static_cast<unsigned int>(sizeof(std::uint32_t) * cop_triangle_ids.size()));
    m_cop_vao->add_buffer(vb_cop_ID, layout_cop_ids);

    return 0;
}

std::vector<float> CoPShader::shade_satellite(glm::vec3 v_rel_hat, float bounding_sphere_radius, std::span<const unsigned int> num_triangles_per_mesh, std::span<const glm::mat4> model_matrices) {
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

	//render triangles and cops to Framebuffer
    m_shader->bind();
	m_triangle_vao->bind();
	unsigned int triangle_offset = 0;
    for (int i = 0; i < num_triangles_per_mesh.size(); i++) {
		glm::mat4 model = model_matrices[i];
        glm::mat4 u_MVP = orthoProj * view * model;
        m_shader->set_uniform_mat4f("u_MVP", u_MVP);
        glDrawArrays(GL_TRIANGLES, triangle_offset, static_cast<GLsizei>(num_triangles_per_mesh[i] * 3));
        triangle_offset += num_triangles_per_mesh[i] * 3;
    }
    m_triangle_vao->unbind();
    m_shader->unbind();

    m_point_shader->bind();
    m_cop_vao->bind();
    unsigned int cop_offset = 0;
    for (int i = 0; i < num_triangles_per_mesh.size(); i++) {
        glm::mat4 model = model_matrices[i];
        glm::mat4 u_MVP = orthoProj * view * model;
        m_point_shader->set_uniform_mat4f("u_MVP", u_MVP);
        glDrawArrays(GL_POINTS, cop_offset, static_cast<GLsizei>(num_triangles_per_mesh[i]));
        cop_offset += num_triangles_per_mesh[i];
    }
    m_cop_vao->unbind();
    m_point_shader->unbind();

    // Make the rendered IDs visible to the reduction shader, then reduce on the GPU:
    // only one flag per triangle is read back, not the whole NUM_PIXEL^2 image.
    GLCall(glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT));
    m_frame_buffer->unbind();
    return m_visibility_reducer->reduce(m_ID_texture, NUM_PIXEL);
};
