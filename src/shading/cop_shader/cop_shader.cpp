#include "cop_shader.h"
#define FMT_UNICODE 0 // avoid error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <glm/glm.hpp>

//custom abstractions
#include "vertex_buffer.h"
#include "gl_helpers.h"
#include "flow_camera.h"
#include "leeward_classification.h"
#include "vertex_buffer_layout.h"

// embedded shader headers
#include "cop_shader/shaders/id_shader.h"

namespace vat::shading {

// The gl:: wrappers are this layer's own building blocks; unqualified use keeps
// the OpenGL call sites readable. TU-local, so it never leaks through a header.
using namespace gl;

CoPShader::CoPShader(unsigned int num_pixel)
    : NUM_PIXEL(num_pixel) {
}

CoPShader::~CoPShader() = default;

int CoPShader::set_vertices(std::span<const float> vertices, std::span<const std::uint32_t> triangleIDs) {
    m_numTriangles = static_cast<unsigned int>(triangleIDs.size() / 3); //assume 3 vertices per triangle
    if (m_numTriangles > MAX_TRIANGLES) {
        SPDLOG_ERROR("Number of triangles ({}) exceeds the maximum supported ({}).", m_numTriangles, MAX_TRIANGLES);
        return -1;
    }

    // Depth only: visibility is marked in the SSBO, so no colour attachment is needed.
    SPDLOG_DEBUG("CoP shading: {}x{} px depth buffer", NUM_PIXEL, NUM_PIXEL);
    m_frame_buffer = std::make_unique<FrameBuffer>(NUM_PIXEL, NUM_PIXEL, 1u);
    m_frame_buffer->UnBind();
    if (!m_frame_buffer->is_complete()) {
        SPDLOG_ERROR("Could not create the {}x{} depth buffer.", NUM_PIXEL, NUM_PIXEL);
        m_frame_buffer.reset();
        return -1;
    }

    // Fixed for this pipeline's own context, so set once rather than every frame.
    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glCullFace(GL_BACK));
    GLCall(glFrontFace(GL_CCW)); // Counter-clockwise is front-facing

    m_visibility = std::make_unique<VisibilityReducer>(m_numTriangles);

    // Create shader programs from embedded sources
    m_shader.reset(new Shader(cop_glsl::triangle_vertex_shader, cop_glsl::triangle_fragment_shader, true));
    m_shader->Unbind();
    m_point_shader.reset(new Shader(cop_glsl::point_vertex_shader, cop_glsl::point_fragment_shader, true));
    m_point_shader->Unbind();

    // Triangles: positions only
    m_triangle_vao.reset(new VertexArray());
    VertexBufferLayout layoutVertices;
    layoutVertices.Push<float>(3);           // vec3 position
    VertexBuffer vb(vertices.data(), static_cast<unsigned int>(sizeof(float) * vertices.size()));
    m_triangle_vao->AddBuffer(vb, layoutVertices);

    //compute triangle centroids from vertices
    std::vector<float> cop(m_numTriangles*3,0);
    std::vector<float> cop_normals(m_numTriangles*3,0);
    for (unsigned int i = 0; i < m_numTriangles; i++) {
        cop[i*3] = (vertices[i*9] + vertices[i*9+3] + vertices[i*9+6]) / 3.0f;
        cop[i*3+1] = (vertices[i*9+1] + vertices[i*9+4] + vertices[i*9+7]) / 3.0f;
        cop[i*3+2] = (vertices[i*9+2] + vertices[i*9+5] + vertices[i*9+8]) / 3.0f;
        //compute unit normal for cop assume CCW winding order of vertices
        glm::vec3 v0(vertices[i*9], vertices[i*9+1], vertices[i*9+2]);
        glm::vec3 v1(vertices[i*9+3], vertices[i*9+4], vertices[i*9+5]);
        glm::vec3 v2(vertices[i*9+6], vertices[i*9+7], vertices[i*9+8]);
        glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        cop_normals[i*3] = normal.x;
        cop_normals[i*3+1] = normal.y;
        cop_normals[i*3+2] = normal.z;
    }

    m_cop_vao.reset(new VertexArray());
    VertexBufferLayout layoutCop;
    layoutCop.Push<float>(3);
    VertexBuffer vbCop(cop.data(), static_cast<unsigned int>(sizeof(float) * cop.size()));
    m_cop_vao->AddBuffer(vbCop, layoutCop);

    //set triangle IDs for Cops
    VertexBufferLayout layout_cop_ids;
    layout_cop_ids.Push<unsigned int>(1);         // triangle ID
    //since trianlgeIds has one value for each vertex cop ids should take every 3rd entry from triangleIds
    std::vector<std::uint32_t> cop_triangle_ids(m_numTriangles,0);
    for (unsigned int i = 0; i < m_numTriangles; i++) {
        cop_triangle_ids[i] = triangleIDs[i*3];
    }
    VertexBuffer vb_cop_ID(cop_triangle_ids.data(), static_cast<unsigned int>(sizeof(std::uint32_t) * cop_triangle_ids.size()));
    m_cop_vao->AddBuffer(vb_cop_ID, layout_cop_ids);

    //set normals for Cops
    VertexBufferLayout layout_cop_normals;
    layout_cop_normals.Push<float>(3);
    VertexBuffer vb_cop_normals(cop_normals.data(), static_cast<unsigned int>(sizeof(float) * cop_normals.size()));
    m_cop_vao->AddBuffer(vb_cop_normals, layout_cop_normals);

    return 0;
}

std::vector<float> CoPShader::shade_geometry(glm::vec3 v_rel_hat, float bounding_sphere_radius, std::span<const unsigned int> num_triangles_per_mesh, std::span<const glm::mat4> model_matrices) {
    if (!m_frame_buffer || !m_shader || !m_point_shader || !m_triangle_vao || !m_cop_vao || !m_visibility) {
        throw std::logic_error("CoPShader::shade_geometry called before a successful set_vertices");
    }
    const FlowCamera camera = make_flow_camera(v_rel_hat, bounding_sphere_radius);
    const glm::mat4& orthoProj = camera.projection;
    const glm::mat4& view = camera.view;

    // Puts the depth state back on every exit path, including an exception from a draw.
    struct DepthStateGuard {
        ~DepthStateGuard() {
            glDepthMask(GL_TRUE);
            glDepthFunc(GL_LESS);
        }
    } depth_state_guard;

    m_frame_buffer->Bind();
    // The default viewport is the hidden window's size, which the OS may clamp to
    // the screen; without this, num_pixel above the screen size is silently lost.
    GLCall(glViewport(0, 0, static_cast<GLsizei>(NUM_PIXEL), static_cast<GLsizei>(NUM_PIXEL)));
    m_frame_buffer->ClearDepth();
    m_visibility->begin(0);

    // Pass 1: depth of the triangles only. The SSBO stays untouched, because a centroid
    // drawn before its occluder would otherwise be marked.
    GLCall(glDepthFunc(GL_LESS));
    GLCall(glDepthMask(GL_TRUE));
    m_shader->Bind();
    m_triangle_vao->Bind();
    unsigned int triangle_offset = 0;
    for (size_t i = 0; i < num_triangles_per_mesh.size(); i++) {
        glm::mat4 model = model_matrices[i];
        glm::mat4 u_MVP = orthoProj * view * model;
        m_shader->setUniformMat4f("u_MVP", u_MVP);
        GLCall(glDrawArrays(GL_TRIANGLES, static_cast<GLint>(triangle_offset), static_cast<GLsizei>(num_triangles_per_mesh[i] * 3)));
        triangle_offset += num_triangles_per_mesh[i] * 3;
    }
    m_triangle_vao->Unbind();
    m_shader->Unbind();

    // Pass 2: every centroid that matches or beats the final depth marks its triangle.
    GLCall(glDepthFunc(GL_LEQUAL));
    GLCall(glDepthMask(GL_FALSE));
    m_point_shader->Bind();
    m_point_shader->SetUniform1f("u_normal_offset", NORMAL_OFFSET);
    m_point_shader->SetUniform3f("u_v_rel_hat", v_rel_hat);
    m_point_shader->SetUniform1f("u_edge_on_threshold", edge_on_threshold(glm::length(v_rel_hat)));
    m_cop_vao->Bind();
    unsigned int cop_offset = 0;
    for (size_t i = 0; i < num_triangles_per_mesh.size(); i++) {
        glm::mat4 model = model_matrices[i];
        glm::mat4 u_MVP = orthoProj * view * model;
        // Same normal transform as RotatableMeshGeometry applies on the CPU.
        const glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(model)));
        m_point_shader->setUniformMat4f("u_MVP", u_MVP);
        m_point_shader->setUniformMat3f("u_normal_matrix", normal_matrix);
        GLCall(glDrawArrays(GL_POINTS, static_cast<GLint>(cop_offset), static_cast<GLsizei>(num_triangles_per_mesh[i])));
        cop_offset += num_triangles_per_mesh[i];
    }
    m_cop_vao->Unbind();
    m_point_shader->Unbind();
    m_frame_buffer->UnBind();

    return m_visibility->read_back();
}

} // namespace vat::shading
