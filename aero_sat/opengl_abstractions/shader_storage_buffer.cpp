#include "shader_storage_buffer.h"
#include "gl_helpers.h"
ShaderStorageBuffer::ShaderStorageBuffer(const void *data, size_t size) {
    GLCall(glGenBuffers(1, &m_shader_storage_buffer_id));
    bind();
    GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW));
    unbind();
}

ShaderStorageBuffer::~ShaderStorageBuffer() {
    unbind();
    GLCall(glDeleteBuffers(1, &m_shader_storage_buffer_id));
}

void ShaderStorageBuffer::bind() {
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_shader_storage_buffer_id));
}

void ShaderStorageBuffer::unbind() {
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
}

void ShaderStorageBuffer::bind_base(unsigned int binding_point) {
    GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding_point, m_shader_storage_buffer_id));
}

void ShaderStorageBuffer::get_data(void *data, size_t size) {
    bind();
    GLCall(glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data));
    unbind();
}
