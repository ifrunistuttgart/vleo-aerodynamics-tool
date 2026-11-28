#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include <stdexcept>
#include <vector>

#include "frame_buffer.h"
#include "gl_helpers.h"

namespace vat::gl {

FrameBuffer::FrameBuffer(unsigned int width, unsigned int heigth, unsigned int num_msaa_samples)
    : m_width(width), m_height(heigth), m_num_msaa_samples(num_msaa_samples) {
    const bool multisample = num_msaa_samples > 1;

    // initialize Framebuffer
    GLCall(glGenFramebuffers(1, &m_frame_buffer_id));
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer_id));

    // no colour attachment yet: depth-only until attach_color_textures is called
    GLCall(glDrawBuffer(GL_NONE));
    GLCall(glReadBuffer(GL_NONE));

    // attach Depthbuffer; 0 MSAA samples is the same storage as glRenderbufferStorage
    GLCall(glGenRenderbuffers(1, &m_depth_buffer_id));
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_depth_buffer_id));
    // Allocation can legitimately fail for large num_pixel * num_msaa_samples, so check the error
    // here instead of letting GLCall treat it as a programming error.
    GLClearError();
    if (multisample) {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, static_cast<GLsizei>(num_msaa_samples), GL_DEPTH_COMPONENT24,
                                         static_cast<GLsizei>(width), static_cast<GLsizei>(heigth));
    } else {
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, static_cast<GLsizei>(width),
                              static_cast<GLsizei>(heigth));
    }

    const GLenum alloc_error = glGetError();
    m_depth_allocated = alloc_error == GL_NO_ERROR;
    if (!m_depth_allocated) {
        SPDLOG_WARN("Framebuffer depth buffer {}x{} with {} sample(s) failed (GL error {})", width, heigth,
                    num_msaa_samples, alloc_error);
    }
    GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_buffer_id));
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));

    check_completeness();
}

void FrameBuffer::check_completeness() {
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    m_complete = m_depth_allocated && status == GL_FRAMEBUFFER_COMPLETE;
    if (!m_complete && m_depth_allocated) {
        SPDLOG_WARN("Framebuffer {}x{} with {} sample(s) not complete (status {})", m_width, m_height,
                    m_num_msaa_samples, status);
    }
}

void FrameBuffer::attach_color_textures(std::span<const std::shared_ptr<Texture2D>> color_textures) {
    for (const std::shared_ptr<Texture2D>& texture : color_textures) {
        if (!texture || !texture->is_valid() || static_cast<unsigned int>(texture->get_width()) != m_width ||
            static_cast<unsigned int>(texture->get_height()) != m_height ||
            static_cast<unsigned int>(texture->get_num_samples()) != m_num_msaa_samples) {
            throw std::invalid_argument(
                "FrameBuffer: colour textures must be valid and match the framebuffer's size and sample count");
        }
    }

    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer_id));
    // Detach what an earlier call attached.
    for (unsigned int i = 0; i < m_num_color_attachments; ++i) {
        GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0));
    }
    m_num_color_attachments = static_cast<unsigned int>(color_textures.size());
    m_color_textures.assign(color_textures.begin(), color_textures.end());

    if (m_num_color_attachments == 0) {
        GLCall(glDrawBuffer(GL_NONE));
        GLCall(glReadBuffer(GL_NONE));
    } else {
        std::vector<GLenum> draw_buffers;
        for (unsigned int i = 0; i < m_num_color_attachments; ++i) {
            GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, color_textures[i]->get_target(),
                                          color_textures[i]->get_texture_id(), 0));
            draw_buffers.push_back(GL_COLOR_ATTACHMENT0 + i);
        }
        GLCall(glDrawBuffers(static_cast<GLsizei>(draw_buffers.size()), draw_buffers.data()));
    }
    check_completeness();
}

FrameBuffer::~FrameBuffer() {
    if (m_frame_buffer_id != 0) {
        GLCall(glDeleteFramebuffers(1, &m_frame_buffer_id));
    }
    if (m_depth_buffer_id != 0) {
        GLCall(glDeleteRenderbuffers(1, &m_depth_buffer_id));
    }
};

void FrameBuffer::bind() const { GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer_id)); };

void FrameBuffer::unbind() const { GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0)); }

void FrameBuffer::clear() const {
    for (const std::shared_ptr<Texture2D>& texture : m_color_textures) {
        texture->clear();
    }
    clear_depth();
}

void FrameBuffer::clear_depth() const {
    GLCall(glDepthMask(GL_TRUE)); // glClear honours the depth write mask
    GLCall(glClear(GL_DEPTH_BUFFER_BIT));
}

void FrameBuffer::resolve_color_into(const FrameBuffer& target) const {
    if (target.m_width != m_width || target.m_height != m_height ||
        target.m_num_color_attachments != m_num_color_attachments || target.m_num_msaa_samples > 1) {
        throw std::invalid_argument(
            "FrameBuffer::resolve_color_into: target must be single-sample with the same size and attachments");
    }
    const auto width = static_cast<GLint>(m_width);
    const auto height = static_cast<GLint>(m_height);
    GLCall(glBindFramebuffer(GL_READ_FRAMEBUFFER, m_frame_buffer_id));
    GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.m_frame_buffer_id));
    for (unsigned int i = 0; i < m_num_color_attachments; ++i) {
        GLCall(glReadBuffer(GL_COLOR_ATTACHMENT0 + i));
        GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT0 + i));
        GLCall(glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST));
    }
    // Put back what attach_color_textures set: every attachment a draw buffer, no read buffer.
    std::vector<GLenum> draw_buffers;
    for (unsigned int i = 0; i < m_num_color_attachments; ++i) {
        draw_buffers.push_back(GL_COLOR_ATTACHMENT0 + i);
    }
    GLCall(glDrawBuffers(static_cast<GLsizei>(draw_buffers.size()), draw_buffers.data()));
    GLCall(glReadBuffer(GL_NONE));
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

} // namespace vat::gl
