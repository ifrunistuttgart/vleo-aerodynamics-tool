#pragma once
#include <vector>
#include <type_traits>
#include <GL/glew.h>
#include "opengl/gl_helpers.h"


struct VertexBufferElement {
	unsigned int type;
	unsigned int count;
	unsigned char normalized;

	static unsigned int GetSizeOfType(unsigned int type)
	{
		switch (type)
		{
		case GL_FLOAT: return 4;
		case GL_UNSIGNED_INT: return 4;
		}
		ASSERT(false)
		return 0;
	}
};

class VertexBufferLayout {
private:
	std::vector<VertexBufferElement> m_Elements;
	unsigned int m_Stride;

public:
	VertexBufferLayout()
		:m_Stride(0) { }

	template<typename T>
	void Push(unsigned int count)
	{
		if constexpr (std::is_same_v<T, float>)
		{
			m_Elements.push_back(VertexBufferElement{GL_FLOAT, count, GL_FALSE});
			m_Stride += count *  VertexBufferElement::GetSizeOfType(GL_FLOAT);
		}
		else if constexpr (std::is_same_v<T, unsigned int>)
		{
			m_Elements.push_back(VertexBufferElement{ GL_UNSIGNED_INT, count, GL_FALSE });
			m_Stride += count * VertexBufferElement::GetSizeOfType(GL_UNSIGNED_INT);
		}
		else
		{
			static_assert(sizeof(T) == 0, "Unsupported type for VertexBufferLayout::Push");
		}
	}

	inline const std::vector<VertexBufferElement> GetElements() const& { return m_Elements; };
	inline unsigned int GetStride() const { return m_Stride; }
};