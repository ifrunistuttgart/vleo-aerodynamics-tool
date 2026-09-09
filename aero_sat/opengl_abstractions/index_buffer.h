#pragma once

class IndexBuffer
{
private:
	unsigned int m_index_buffer_id;
	unsigned int m_count;
public:
	IndexBuffer(const unsigned int* data, unsigned int count);
	~IndexBuffer();

	void bind() const;
	void unbind() const;

	[[nodiscard]] inline unsigned int get_count() const { return m_count; }
};