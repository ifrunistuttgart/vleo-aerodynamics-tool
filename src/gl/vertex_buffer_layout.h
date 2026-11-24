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
    static unsigned int GetSizeOfType(unsigned int type) {
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

/** @brief Interleaved attribute layout of one vertex buffer, in Push() order. */
class VertexBufferLayout {
private:
    std::vector<VertexBufferElement> m_Elements;
    unsigned int m_Stride;

public:
    /** @brief Creates an empty layout. */
    VertexBufferLayout() : m_Stride(0) {}

    /**
     * @brief Appends an attribute of `count` components of type T.
     * @tparam T float or unsigned int; any other type fails to compile.
     * @param count Components per vertex.
     */
    template <typename T> void Push(unsigned int count) {
        if constexpr (std::is_same_v<T, float>) {
            m_Elements.push_back(VertexBufferElement{GL_FLOAT, count, GL_FALSE});
            m_Stride += count * VertexBufferElement::GetSizeOfType(GL_FLOAT);
        } else if constexpr (std::is_same_v<T, unsigned int>) {
            m_Elements.push_back(VertexBufferElement{GL_UNSIGNED_INT, count, GL_FALSE});
            m_Stride += count * VertexBufferElement::GetSizeOfType(GL_UNSIGNED_INT);
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type for VertexBufferLayout::Push");
        }
    }

    /** @brief The attributes in Push() order. */
    inline const std::vector<VertexBufferElement> GetElements() const& { return m_Elements; };
    /** @brief Bytes per vertex over all attributes. */
    inline unsigned int GetStride() const { return m_Stride; }
};

} // namespace vat::gl
