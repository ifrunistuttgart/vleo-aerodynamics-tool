#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include <stdexcept>
#include <vector>

#include "frame_buffer.h"
#include "gl_helpers.h"

namespace vat::gl {

FrameBuffer::FrameBuffer(unsigned int width, unsigned int heigth, unsigned int num_msaa_samples)
	: m_width(width), m_height(heigth), m_num_msaa_samples(num_msaa_samples)
{
	const bool multisample = num_msaa_samples > 1;

	//initialize Framebuffer
	GLCall(glGenFramebuffers(1, &m_FrameBufferID));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID));

	//no colour attachment yet: depth-only until attach_color_textures is called
	GLCall(glDrawBuffer(GL_NONE));
	GLCall(glReadBuffer(GL_NONE));

	//attach Depthbuffer; 0 MSAA samples is the same storage as glRenderbufferStorage
	GLCall(glGenRenderbuffers(1, &m_DepthBufferID));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBufferID));
	// Allocation can legitimately fail for large num_pixel * num_msaa_samples, so check the error
	// here instead of letting GLCall treat it as a programming error.
	GLClearError();
	if (multisample) {
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, static_cast<GLsizei>(num_msaa_samples),
			GL_DEPTH_COMPONENT24, static_cast<GLsizei>(width), static_cast<GLsizei>(heigth));
	} else {
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, static_cast<GLsizei>(width), static_cast<GLsizei>(heigth));
	}

	const GLenum alloc_error = glGetError();
	m_depth_allocated = alloc_error == GL_NO_ERROR;
	if (!m_depth_allocated) {
		SPDLOG_WARN("Framebuffer depth buffer {}x{} with {} sample(s) failed (GL error {})",
			width, heigth, num_msaa_samples, alloc_error);
	}
	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_DepthBufferID));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));

	check_completeness();
}

void FrameBuffer::check_completeness()
{
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	m_complete = m_depth_allocated && status == GL_FRAMEBUFFER_COMPLETE;
	if (!m_complete && m_depth_allocated) {
		SPDLOG_WARN("Framebuffer {}x{} with {} sample(s) not complete (status {})",
			m_width, m_height, m_num_msaa_samples, status);
	}
}

void FrameBuffer::attach_color_textures(std::span<const std::shared_ptr<Texture2D>> colorTextures)
{
	for (const std::shared_ptr<Texture2D>& texture : colorTextures) {
		if (!texture || !texture->is_valid() || static_cast<unsigned int>(texture->get_width()) != m_width ||
			static_cast<unsigned int>(texture->get_height()) != m_height ||
			static_cast<unsigned int>(texture->get_num_samples()) != m_num_msaa_samples) {
			throw std::invalid_argument("FrameBuffer: colour textures must be valid and match the framebuffer's size and sample count");
		}
	}

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID));
	// Detach what an earlier call attached.
	for (unsigned int i = 0; i < m_NumColorAttachments; ++i) {
		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0));
	}
	m_NumColorAttachments = static_cast<unsigned int>(colorTextures.size());
	m_color_textures.assign(colorTextures.begin(), colorTextures.end());

	if (m_NumColorAttachments == 0) {
		GLCall(glDrawBuffer(GL_NONE));
		GLCall(glReadBuffer(GL_NONE));
	} else {
		std::vector<GLenum> drawBuffers;
		for (unsigned int i = 0; i < m_NumColorAttachments; ++i) {
			GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, colorTextures[i]->get_target(),
				colorTextures[i]->get_texture_id(), 0));
			drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
		}
		GLCall(glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data()));
	}
	check_completeness();
}

FrameBuffer::~FrameBuffer()
{
	if (m_FrameBufferID != 0) {
		GLCall(glDeleteFramebuffers(1, &m_FrameBufferID));
	}
	if (m_DepthBufferID != 0) {
		GLCall(glDeleteRenderbuffers(1, &m_DepthBufferID));
	}
};

void FrameBuffer::Bind() const
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID));
};

void FrameBuffer::UnBind() const
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void FrameBuffer::Clear() const
{
	for (const std::shared_ptr<Texture2D>& texture : m_color_textures) {
		texture->clear();
	}
	ClearDepth();
}

void FrameBuffer::ClearDepth() const
{
	GLCall(glDepthMask(GL_TRUE)); // glClear honours the depth write mask
	GLCall(glClear(GL_DEPTH_BUFFER_BIT));
}

} // namespace vat::gl
