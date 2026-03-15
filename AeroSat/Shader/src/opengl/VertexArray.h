#pragma once
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"


class VertexArray {
private:
	unsigned int m_vertex_array_id;
	unsigned int m_attrib_index;
public:
	VertexArray();
	~VertexArray();

	void AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout);

	void Bind() const;
	void Unbind() const;
};