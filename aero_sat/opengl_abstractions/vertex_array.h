#pragma once
#include "vertex_buffer.h"
#include "vertex_buffer_layout.h"


class VertexArray {
private:
	unsigned int m_vertex_array_id;
	unsigned int m_attrib_index;
public:
	VertexArray();
	~VertexArray();

	void add_buffer(const VertexBuffer& vb, const VertexBufferLayout& layout);

	void bind() const;
	void unbind() const;
};