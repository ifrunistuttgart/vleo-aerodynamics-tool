#pragma once

namespace vat::gl {

/**
 * @brief Owns one GL_ARRAY_BUFFER filled once with GL_STATIC_DRAW data.
 *
 * A context must be current when it is created and destroyed.
 */
class VertexBuffer {
private:
    unsigned int m_vertex_buffer_id;

public:
    /**
     * @brief Creates the buffer and uploads the data; leaves it bound to GL_ARRAY_BUFFER.
     * @param data Data to upload.
     * @param size Data size in bytes.
     */
    VertexBuffer(const void* data, unsigned int size);
    ~VertexBuffer();

    /** @brief Binds the buffer to GL_ARRAY_BUFFER. */
    void bind() const;
    /** @brief Binds no buffer to GL_ARRAY_BUFFER. */
    void unbind() const;
};

} // namespace vat::gl
