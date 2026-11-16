#pragma once

namespace vat::gl {

/**
 * Which kind of access a barrier makes visible to later commands. Combine with `|`.
 * (Named Barrier because MemoryBarrier is a macro in the Windows headers.)
 */
enum class Barrier : unsigned int {
    None = 0,
    ShaderStorage = 1 << 0, ///< shader reads/writes of SSBOs
    BufferUpdate = 1 << 1,  ///< buffer reads on the CPU side, e.g. glGetBufferSubData
    TextureUpdate = 1 << 2, ///< texture reads and writes through image units or the CPU
    TextureFetch = 1 << 3,  ///< texture fetches in shaders after rendering to the texture
};

constexpr Barrier operator|(Barrier a, Barrier b) {
    return static_cast<Barrier>(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

/// glMemoryBarrier for the given kinds of access.
void memory_barrier(Barrier barriers);

} // namespace vat::gl
