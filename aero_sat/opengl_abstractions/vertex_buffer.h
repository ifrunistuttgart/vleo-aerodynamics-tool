#pragma once

class VertexBuffer
{
private:
	unsigned int m_vertex_buffer_id;
public:
	VertexBuffer(const void* data, unsigned int size);
	~VertexBuffer();

	void bind() const;
	void unbind() const;
};