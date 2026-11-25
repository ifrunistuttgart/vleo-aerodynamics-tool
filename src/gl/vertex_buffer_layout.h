#pragma once
#include <vector>
#include <type_traits>
#include <GL/glew.h>
#include "gl_helpers.h"

namespace vat::gl {

/** @brief One interleaved vertex attribute of a VertexBufferLayout. */
struct VertexBufferElement {
    unsigned int type;        /**< GL component type, GL_FLOAT or GL_UNSIGNED_INT. */
    unsigned int count;       /**< Components per vertex. */
    unsigned char normalized; /**< GL_TRUE to normalise integer data to [0, 1]. */

    /**
     * @brief Size of one component in bytes.
     * @param type GL_FLOAT or GL_UNSIGNED_INT; any other type triggers ASSERT and returns 0.
     * @return 4 for the supported types.
     */
    static unsigned int get_size_of_type(unsigned int type) {
        switch (type) {
        case GL_FLOAT:
            return 4;
        case GL_UNSIGNED_INT:
            return 4;
        }
        ASSERT(false)
        return 0;
    }
};

/** @brief Interleaved attribute layout of one vertex buffer, in push() order. */
class VertexBufferLayout {
private:
    std::vector<VertexBufferElement> m_elements;
    unsigned int m_stride;

public:
    /** @brief Creates an empty layout. */
    VertexBufferLayout() : m_stride(0) {}

    /**
     * @brief Appends an attribute of `count` components of type T.
     * @tparam T float or unsigned int; any other type fails to compile.
     * @param count Components per vertex.
     */
    template <typename T> void push(unsigned int count) {
        if constexpr (std::is_same_v<T, float>) {
            m_elements.push_back(VertexBufferElement{GL_FLOAT, count, GL_FALSE});
            m_stride += count * VertexBufferElement::get_size_of_type(GL_FLOAT);
        } else if constexpr (std::is_same_v<T, unsigned int>) {
            m_elements.push_back(VertexBufferElement{GL_UNSIGNED_INT, count, GL_FALSE});
            m_stride += count * VertexBufferElement::get_size_of_type(GL_UNSIGNED_INT);
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type for VertexBufferLayout::push");
        }
    }

    /** @brief The attributes in push() order. */
    inline const std::vector<VertexBufferElement> get_elements() const& { return m_elements; };
    /** @brief Bytes per vertex over all attributes. */
    inline unsigned int get_stride() const { return m_stride; }
};

} // namespace vat::gl
