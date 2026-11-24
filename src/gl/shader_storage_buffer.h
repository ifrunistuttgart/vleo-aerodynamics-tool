#pragma once
#include <cstddef>
#include <span>

namespace vat::gl {

/// How the GPU buffer's data is used; a hint to the driver. The values map to GL usage enums.
enum class BufferUsage { StaticDraw, DynamicRead };

/**
 * RAII wrapper of a shader storage buffer object (SSBO) of a fixed size.
 *
 * Move-only. A context must be current when the buffer is created, used and destroyed.
 * The GL enums stay out of this header so it does not depend on GLEW.
 */
class ShaderStorageBuffer {
public:
    /// Allocates `size_bytes` of uninitialised storage.
    explicit ShaderStorageBuffer(std::size_t size_bytes, BufferUsage usage = BufferUsage::DynamicRead);
    /// Allocates storage the size of `data` and uploads it.
    explicit ShaderStorageBuffer(std::span<const std::byte> data, BufferUsage usage = BufferUsage::StaticDraw);
    ~ShaderStorageBuffer();

    ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;
    ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;
    ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept;
    ShaderStorageBuffer& operator=(ShaderStorageBuffer&& other) noexcept;

    /// Binds the whole buffer to the shader's `layout(binding = N)` slot.
    void bind_base(unsigned int binding) const;

    /// Sets every 32-bit word of the buffer to zero.
    void clear() const;

    /**
     * Copies the first `out.size()` bytes into `out`; waits for the GPU writes to this buffer.
     * Call memory_barrier() first if a shader wrote to it. Throws std::out_of_range if `out`
     * is larger than the buffer.
     */
    void read_back(std::span<std::byte> out) const;

    template <class T> void read_back(std::span<T> out) const { read_back(std::as_writable_bytes(out)); }

    [[nodiscard]] unsigned int get_buffer_id() const { return m_buffer_id; }
    [[nodiscard]] std::size_t get_size_bytes() const { return m_size_bytes; }

private:
    void allocate(std::span<const std::byte> data, std::size_t size_bytes, BufferUsage usage);

    unsigned int m_buffer_id = 0;
    std::size_t m_size_bytes = 0;
};

} // namespace vat::gl
