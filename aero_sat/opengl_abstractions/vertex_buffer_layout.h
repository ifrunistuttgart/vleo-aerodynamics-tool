#pragma once
#include <vector>
#include <type_traits>
#include <GL/glew.h>
#include "gl_helpers.h"


struct VertexBufferElement {
	unsigned int type;
	unsigned int count;
	unsigned char normalized;

	static unsigned int get_size_of_type(unsigned int type)
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
	std::vector<VertexBufferElement> m_elements;
	unsigned int m_stride;

public:
	VertexBufferLayout()
		:m_stride(0) { }

	template<typename T>
	void push(unsigned int count)
	{
		if constexpr (std::is_same_v<T, float>)
		{
			m_elements.push_back(VertexBufferElement{GL_FLOAT, count, GL_FALSE});
			m_stride += count *  VertexBufferElement::get_size_of_type(GL_FLOAT);
		}
		else if constexpr (std::is_same_v<T, unsigned int>)
		{
			m_elements.push_back(VertexBufferElement{ GL_UNSIGNED_INT, count, GL_FALSE });
			m_stride += count * VertexBufferElement::get_size_of_type(GL_UNSIGNED_INT);
		}
		else
		{
			static_assert(sizeof(T) == 0, "Unsupported type for VertexBufferLayout::push");
		}
	}

	[[nodiscard]] inline std::vector<VertexBufferElement> get_elements() const& { return m_elements; };
	[[nodiscard]] inline unsigned int get_stride() const { return m_stride; }
};