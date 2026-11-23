#include "gl_helpers.h"
#include "memory_barrier.h"

namespace vat::gl {

namespace {

bool has(Barrier barriers, Barrier bit) {
    return (static_cast<unsigned int>(barriers) & static_cast<unsigned int>(bit)) != 0;
}

} // namespace

void memory_barrier(Barrier barriers) {
    GLbitfield bits = 0;
    if (has(barriers, Barrier::ShaderStorage)) {
        bits |= GL_SHADER_STORAGE_BARRIER_BIT;
    }
    if (has(barriers, Barrier::BufferUpdate)) {
        bits |= GL_BUFFER_UPDATE_BARRIER_BIT;
    }
    if (has(barriers, Barrier::TextureUpdate)) {
        bits |= GL_TEXTURE_UPDATE_BARRIER_BIT;
    }
    if (has(barriers, Barrier::TextureFetch)) {
        bits |= GL_TEXTURE_FETCH_BARRIER_BIT;
    }
    if (bits != 0) {
        GLCall(glMemoryBarrier(bits));
    }
}

} // namespace vat::gl
