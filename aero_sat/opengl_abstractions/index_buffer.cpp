#include "index_buffer.h"
#include "gl_helpers.h"

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

IndexBuffer::IndexBuffer(const unsigned int* data, unsigned int count)
	: m_count(count)
{
	ASSERT(sizeof(unsigned int) == sizeof(GLuint));

	if (data == nullptr || count == 0) {
		SPDLOG_WARN("IndexBuffer created with {} data and count={}", data == nullptr ? "null" : "non-null", count);
	}

	GLCall(glGenBuffers(1, &m_index_buffer_id));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer_id));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_STATIC_DRAW));
}

IndexBuffer::~IndexBuffer()
{
	GLCall(glDeleteBuffers(1, &m_index_buffer_id));
}

void IndexBuffer::bind() const
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer_id));
}

void IndexBuffer::unbind() const
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}