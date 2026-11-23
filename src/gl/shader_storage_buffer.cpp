#include <stdexcept>
#include <utility>

#include "gl_helpers.h"
#include "shader_storage_buffer.h"

namespace vat::gl {

namespace {

GLenum to_gl_usage(BufferUsage usage) {
    return usage == BufferUsage::StaticDraw ? GL_STATIC_DRAW : GL_DYNAMIC_READ;
}

} // namespace

ShaderStorageBuffer::ShaderStorageBuffer(std::size_t size_bytes, BufferUsage usage) {
    allocate({}, size_bytes, usage);
}

ShaderStorageBuffer::ShaderStorageBuffer(std::span<const std::byte> data, BufferUsage usage) {
    allocate(data, data.size(), usage);
}

void ShaderStorageBuffer::allocate(std::span<const std::byte> data, std::size_t size_bytes, BufferUsage usage) {
    m_size_bytes = size_bytes;
    GLCall(glGenBuffers(1, &m_buffer_id));
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer_id));
    GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(size_bytes),
                        data.empty() ? nullptr : data.data(), to_gl_usage(usage)));
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
}

ShaderStorageBuffer::~ShaderStorageBuffer() {
    if (m_buffer_id != 0) {
        GLCall(glDeleteBuffers(1, &m_buffer_id));
    }
}

ShaderStorageBuffer::ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept
    : m_buffer_id(std::exchange(other.m_buffer_id, 0u)),
      m_size_bytes(std::exchange(other.m_size_bytes, std::size_t{0})) {}

ShaderStorageBuffer& ShaderStorageBuffer::operator=(ShaderStorageBuffer&& other) noexcept {
    if (this != &other) {
        if (m_buffer_id != 0) {
            glDeleteBuffers(1, &m_buffer_id);
        }
        m_buffer_id = std::exchange(other.m_buffer_id, 0u);
        m_size_bytes = std::exchange(other.m_size_bytes, std::size_t{0});
    }
    return *this;
}

void ShaderStorageBuffer::bind_base(unsigned int binding) const {
    GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_buffer_id));
}

void ShaderStorageBuffer::clear() const {
    const GLuint zero = 0;
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer_id));
    GLCall(glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero));
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
}

void ShaderStorageBuffer::read_back(std::span<std::byte> out) const {
    if (out.size() > m_size_bytes) {
        throw std::out_of_range("ShaderStorageBuffer::read_back: more bytes requested than the buffer holds");
    }
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer_id));
    GLCall(glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr>(out.size()), out.data()));
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0));
}

} // namespace vat::gl
