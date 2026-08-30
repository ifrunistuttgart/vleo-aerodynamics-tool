#include "vertex_array.h"
#include "gl_helpers.h"

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

VertexArray::VertexArray()
	: m_attrib_index(0)
{
	GLCall(glGenVertexArrays(1, &m_vertex_array_id));
}

VertexArray::~VertexArray()
{
	unbind();
}

void VertexArray::add_buffer(const VertexBuffer& vb, const VertexBufferLayout& layout)
{
	bind();
	vb.bind();
	const auto& elements = layout.get_elements();
	if (elements.empty()) {
		SPDLOG_WARN("VertexArray::add_buffer called with empty layout");
	}
	unsigned int offset = 0;
	for (unsigned int i = 0; i < elements.size(); i++)
	{
		const auto& element = elements[i];
		unsigned int attribIndex = m_attrib_index + i;
		GLCall(glEnableVertexAttribArray(attribIndex));

		if (element.type == GL_UNSIGNED_INT || element.type == GL_INT)
		{
			GLCall(glVertexAttribIPointer(attribIndex, element.count, element.type, layout.get_stride(), (const void*)offset));
		}
		else
		{
			GLCall(glVertexAttribPointer(attribIndex, element.count, element.type, element.normalized, layout.get_stride(), (const void*)offset));
		}

		offset += element.count * VertexBufferElement::get_size_of_type(element.type);
	}
	m_attrib_index += elements.size();
	vb.unbind();
	unbind();
}

void VertexArray::bind() const
{
	GLCall(glBindVertexArray(m_vertex_array_id));
}

void VertexArray::unbind() const
{
	GLCall(glBindVertexArray(0));
}