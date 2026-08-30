#include "vertex_buffer.h"
#include "gl_helpers.h"

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

VertexBuffer::VertexBuffer(const void* data, unsigned int size)
{
	if (data == nullptr || size == 0) {
		SPDLOG_WARN("VertexBuffer created with {} data and size={} bytes", data == nullptr ? "null" : "non-null", size);
	}

	GLCall(glGenBuffers(1, &m_vertex_buffer_id));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer_id));
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW));
}

VertexBuffer::~VertexBuffer()
{
	GLCall(glDeleteBuffers(1, &m_vertex_buffer_id));
}

void VertexBuffer::bind() const
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer_id));
}

void VertexBuffer::unbind() const
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}