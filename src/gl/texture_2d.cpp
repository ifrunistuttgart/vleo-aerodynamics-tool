#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "gl_helpers.h"
#include "texture_2d.h"

namespace vat::gl {

namespace {

size_t channel_count(unsigned int format) {
    switch (format) {
    case GL_RED:
    case GL_RED_INTEGER:
        return 1;
    case GL_RG:
    case GL_RG_INTEGER:
        return 2;
    case GL_RGB:
    case GL_RGB_INTEGER:
        return 3;
    case GL_RGBA:
    case GL_RGBA_INTEGER:
        return 4;
    default:
        throw std::logic_error("Texture2D::read_back_float: unsupported pixel format");
    }
}

} // namespace

Texture2D::Texture2D(int width, int height, unsigned int internal_format, unsigned int format, unsigned int dtype,
                     int num_samples, TextureFilter filter)
    : m_width(width), m_height(height), m_internal_format(internal_format), m_format(format), m_dtype(dtype),
      m_num_samples(std::max(1, num_samples)) {
    const GLenum target = get_target();
    GLCall(glGenTextures(1, &m_texture_id));
    GLCall(glBindTexture(target, m_texture_id));

    // Allocation can legitimately fail for large sizes times sample counts, so check the error
    // here instead of letting GLCall treat it as a programming error.
    GLClearError();
    if (is_multisample()) {
        glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_num_samples, internal_format, width, height, GL_TRUE);
    } else {
        glTexStorage2D(GL_TEXTURE_2D, 1, internal_format, width, height);
    }
    const GLenum alloc_error = glGetError();

    if (alloc_error != GL_NO_ERROR) {
        SPDLOG_WARN("Texture2D allocation {}x{} with {} sample(s) failed (GL error {})", width, height, m_num_samples,
                    alloc_error);
        GLCall(glBindTexture(target, 0));
        GLCall(glDeleteTextures(1, &m_texture_id));
        m_texture_id = 0;
        return;
    }
    if (!is_multisample()) {
        // Sampler state does not apply to multisample textures.
        const GLint gl_filter = filter == TextureFilter::Linear ? GL_LINEAR : GL_NEAREST;
        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter));
        GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter));
    }
    GLCall(glBindTexture(target, 0));
}

Texture2D::~Texture2D() {
    if (m_texture_id != 0) {
        GLCall(glDeleteTextures(1, &m_texture_id));
    }
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : m_texture_id(std::exchange(other.m_texture_id, 0u)), m_width(other.m_width), m_height(other.m_height),
      m_internal_format(other.m_internal_format), m_format(other.m_format), m_dtype(other.m_dtype),
      m_num_samples(other.m_num_samples) {}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        if (m_texture_id != 0) {
            glDeleteTextures(1, &m_texture_id);
        }
        m_texture_id = std::exchange(other.m_texture_id, 0u);
        m_width = other.m_width;
        m_height = other.m_height;
        m_internal_format = other.m_internal_format;
        m_format = other.m_format;
        m_dtype = other.m_dtype;
        m_num_samples = other.m_num_samples;
    }
    return *this;
}

unsigned int Texture2D::get_target() const { return is_multisample() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D; }

void Texture2D::bind(unsigned int unit) const {
    GLCall(glActiveTexture(GL_TEXTURE0 + unit));
    GLCall(glBindTexture(get_target(), m_texture_id));
    GLCall(glActiveTexture(GL_TEXTURE0));
}

void Texture2D::unbind(unsigned int unit) const {
    GLCall(glActiveTexture(GL_TEXTURE0 + unit));
    GLCall(glBindTexture(get_target(), 0));
    GLCall(glActiveTexture(GL_TEXTURE0));
}

void Texture2D::bind_as_image(unsigned int unit, unsigned int access) const {
    GLCall(glBindImageTexture(unit, m_texture_id, 0, GL_FALSE, 0, access, m_internal_format));
}

void Texture2D::clear() const {
    if (GLEW_VERSION_4_4 || GLEW_ARB_clear_texture) {
        // Null data means zero in every channel.
        GLCall(glClearTexImage(m_texture_id, 0, m_format, m_dtype, nullptr));
        return;
    }

    // The context is only guaranteed to be 4.3: clear through a scratch framebuffer instead.
    const bool is_integer = m_format == GL_RED_INTEGER || m_format == GL_RG_INTEGER || m_format == GL_RGB_INTEGER ||
                            m_format == GL_RGBA_INTEGER;
    GLint previous_fbo = 0;
    GLCall(glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_fbo));
    GLuint fbo = 0;
    GLCall(glGenFramebuffers(1, &fbo));
    GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo));
    GLCall(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, get_target(), m_texture_id, 0));
    if (is_integer) {
        const GLuint zero[4] = {0, 0, 0, 0};
        GLCall(glClearBufferuiv(GL_COLOR, 0, zero));
    } else {
        const GLfloat zero[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        GLCall(glClearBufferfv(GL_COLOR, 0, zero));
    }
    GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(previous_fbo)));
    GLCall(glDeleteFramebuffers(1, &fbo));
}

std::vector<float> Texture2D::read_back_float() const {
    if (is_multisample()) {
        throw std::logic_error("Texture2D::read_back_float: not possible for a multisample texture");
    }
    std::vector<float> data(static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * channel_count(m_format));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_texture_id));
    GLCall(glPixelStorei(GL_PACK_ALIGNMENT, 4));
    GLCall(glGetTexImage(GL_TEXTURE_2D, 0, m_format, GL_FLOAT, data.data()));
    GLCall(glBindTexture(GL_TEXTURE_2D, 0));
    return data;
}

} // namespace vat::gl
