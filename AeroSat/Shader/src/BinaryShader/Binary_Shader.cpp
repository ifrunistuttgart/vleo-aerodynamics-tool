#pragma once
#include "src/BinaryShader/Binary_Shader.h"
#include <iostream>

//math includes
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

//custom abstractions
#include "src/opengl/VertexBuffer.h"
#include "src/opengl/GLHelpers.h"

// embedded shader headers
#include "res/shaders/ID_shader.h"
#include "res/shaders/Compute_shader.h"
#include "src/opengl/VertexBufferLayout.h"

BinaryShader::BinaryShader(unsigned int num_pixel)
    : NUM_PIXEL(num_pixel)
{

}

BinaryShader::~BinaryShader() {
    m_compute_shader.reset();
    m_shader.reset();
    m_frame_buffer.reset();
    m_vao.reset();

    if (m_ID_texture != 0) {
        GLCall(glDeleteTextures(1, &m_ID_texture));
    }
    if (m_histogramBuffer != 0) {
        GLCall(glDeleteBuffers(1, &m_histogramBuffer));
    }

}

int BinaryShader::set_vertices(float vertices[], size_t lenVertices, unsigned int triangleIDs[], size_t lenTriangleIDs) {
    // framebuffer um ids zu zählen
    GLCall(glGenTextures(1, &m_ID_texture));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_ID_texture));
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, NUM_PIXEL, NUM_PIXEL, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, nullptr));

    m_frame_buffer.reset(new FrameBuffer(m_ID_texture, NUM_PIXEL, NUM_PIXEL));
    m_frame_buffer->UnBind();

    // histogrambuffer für computeshader um pixel zu zählen
    GLCall(glGenBuffers(1, &m_histogramBuffer));
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_histogramBuffer));
    GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_TRIANGLES * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW));
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));

    // Create shader program from embedded sources
    m_shader.reset(new Shader(ID_vertex_shader, ID_fragment_shader, true));
    m_shader->Unbind();
    m_compute_shader.reset(new ComputeShader(Compute_shader, true));
    m_compute_shader->Unbind();

    // Enable depth testing for proper occlusion
    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glDepthFunc(GL_LESS));
    GLCall(glDepthMask(GL_TRUE));

    // Enable face culling
    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glCullFace(GL_BACK));
    GLCall(glFrontFace(GL_CCW)); // Counter-clockwise is front-facing

    m_lenVertices = lenVertices;
	m_numTriangles = lenTriangleIDs;
	m_vao.reset(new VertexArray());
    VertexBufferLayout layoutVertices;
    layoutVertices.Push<float>(3);           // vec3 position
    VertexBuffer vb(vertices, static_cast<unsigned int>(sizeof(float) * lenVertices));
    m_vao->AddBuffer(vb, layoutVertices);

    VertexBufferLayout layoutIDs;
    layoutIDs.Push<unsigned int>(1);         // triangle ID
    VertexBuffer vbID(triangleIDs, static_cast<unsigned int>(sizeof(unsigned int) * lenTriangleIDs));
    m_vao->AddBuffer(vbID, layoutIDs);
    return 0;
}

int BinaryShader::shade_satellite(float triangle_visibility[], glm::vec3 v_rel_hat, float bounding_sphere_radius) {
    //projection matrices
    glm::vec3 camera_position = -v_rel_hat * bounding_sphere_radius;

    glm::mat4 orthoProj = glm::ortho(-bounding_sphere_radius,
        bounding_sphere_radius,
        -bounding_sphere_radius,
        bounding_sphere_radius,
        0.0f,
        2 * bounding_sphere_radius
    );
    glm::mat4 view = glm::lookAt(
        camera_position, // Camera position
        glm::vec3(0.0f, 0.0f, 0.0f), // Look at point
        glm::vec3(0.0f, 1.0f, 0.0f)  //TOdO problem with upvector || to winddirection?
    );
    //glm::mat4 view = glm::mat4(1.0f); // Identity matrix for view
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix for model
    glm::mat4 u_MVP = orthoProj * view * model;



    // PHASE 1: Zu ID-Framebuffer rendern
    m_frame_buffer->Bind();
    m_frame_buffer->Clear();

    // Triangle-IDs rendern
    m_shader->Bind();
	m_vao->Bind();
    m_shader->setUniformMat4f("u_MVP", u_MVP);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_lenVertices / 3));

    // Histogram-Buffer leeren
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_histogramBuffer));
    GLuint* histogramData = (GLuint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    if (histogramData) {
        memset(histogramData, 0, MAX_TRIANGLES * sizeof(GLuint));
        GLCall(glUnmapBuffer(GL_SHADER_STORAGE_BUFFER));
    }

    // ID-Texture für Compute-Shader binden (binding = 0)
    GLCall(glBindImageTexture(0, m_ID_texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R16UI));

    // Histogram-Buffer für Compute-Shader binden (binding = 1)
    GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_histogramBuffer));

    m_compute_shader->Bind();

    // Compute-Shader dispatchen (16x16 Work Groups)
    GLCall(glDispatchCompute((NUM_PIXEL + 15) / 16, (NUM_PIXEL + 15) / 16, 1));

    // Warten bis Compute-Shader fertig ist
    GLCall(glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT));

    // Histogram-Ergebnisse auslesen
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_histogramBuffer));
    histogramData = (GLuint*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

    if (histogramData) {
        //std::cout << "Triangle Histogram:" << std::endl;
        for (size_t i = 0; i < m_numTriangles; i++) {
            if (histogramData[i + 1] > 0) {
                triangle_visibility[i] = true;
                std::cout << "Triangle ID " << i + 1 << ": " << histogramData[i + 1] << " pixels" << std::endl;
            }
        }
        GLCall(glUnmapBuffer(GL_SHADER_STORAGE_BUFFER));
    }
	m_vao->Unbind();
    return 0;
};