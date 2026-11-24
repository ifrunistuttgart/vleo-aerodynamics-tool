#pragma once
#include "vertex_buffer.h"
#include "vertex_buffer_layout.h"

namespace vat::gl {

/**
 * @brief Vertex array object; each AddBuffer() call appends the buffer's attributes at the next
 * free attribute indices, starting at 0.
 *
 * A context must be current when it is created and used.
 */
class VertexArray {
private:
    unsigned int m_vertex_array_id;
    unsigned int m_attrib_index;

public:
    /** @brief Generates the vertex array object. */
    VertexArray();
    ~VertexArray();

    /**
     * @brief Adds the attributes of one vertex buffer, laid out as `layout` describes.
     *
     * Integer attributes use glVertexAttribIPointer. Leaves no vertex array or buffer bound.
     *
     * @param vb Buffer holding the attribute data.
     * @param layout Attribute layout of vb.
     */
    void AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout);

    /** @brief Binds the vertex array object. */
    void Bind() const;
    /** @brief Binds no vertex array object. */
    void Unbind() const;
};

} // namespace vat::gl
