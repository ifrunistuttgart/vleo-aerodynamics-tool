#include "VertexArray.h"
#include "src/opengl/GLHelpers.h"


VertexArray::VertexArray()
	: m_attrib_index(0)
{
	GLCall(glGenVertexArrays(1, &m_vertex_array_id));
}

VertexArray::~VertexArray()
{
	Unbind();
}

void VertexArray::AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout)
{
	Bind();
	vb.Bind();
	const auto& elements = layout.GetElements();
	unsigned int offset = 0;
	for (unsigned int i = 0; i < elements.size(); i++)
	{
		const auto& element = elements[i];
		unsigned int attribIndex = m_attrib_index + i;
		GLCall(glEnableVertexAttribArray(attribIndex));

		if (element.type == GL_UNSIGNED_INT || element.type == GL_INT)
		{
			GLCall(glVertexAttribIPointer(attribIndex, element.count, element.type, layout.GetStride(), (const void*)offset));
		}
		else
		{
			GLCall(glVertexAttribPointer(attribIndex, element.count, element.type, element.normalized, layout.GetStride(), (const void*)offset));
		}

		offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
	}
	m_attrib_index += elements.size();
	vb.Unbind();
	Unbind();
}

void VertexArray::Bind() const
{
	GLCall(glBindVertexArray(m_vertex_array_id));
}

void VertexArray::Unbind() const
{
	GLCall(glBindVertexArray(0));
}