#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include <vector>

#include "frame_buffer.h"
#include "gl_helpers.h"

namespace vat::gl {

FrameBuffer::FrameBuffer(unsigned int texture2D, unsigned int width, unsigned int heigth)
	: FrameBuffer(std::span<const unsigned int>(&texture2D, 1), width, heigth)
{
}

FrameBuffer::FrameBuffer(std::span<const unsigned int> colorTextures, unsigned int width, unsigned int heigth,
	unsigned int num_msaa_samples)
	: m_NumColorAttachments(static_cast<unsigned int>(colorTextures.size()))
{
	const bool multisample = num_msaa_samples > 1;

	//initialize Framebuffer
	GLCall(glGenFramebuffers(1, &m_FrameBufferID));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID));

	//attach Textures (none: depth-only framebuffer)
	if (m_NumColorAttachments == 0) {
		GLCall(glDrawBuffer(GL_NONE));
		GLCall(glReadBuffer(GL_NONE));
	} else {
		const GLenum target = multisample ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
		std::vector<GLenum> drawBuffers;
		for (unsigned int i = 0; i < m_NumColorAttachments; ++i) {
			GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, target, colorTextures[i], 0));
			drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
		}
		GLCall(glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data()));
	}

	//attach Depthbuffer; 0 MSAA samples is the same storage as glRenderbufferStorage
	GLCall(glGenRenderbuffers(1, &m_DepthBufferID));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBufferID));
	// Allocation can legitimately fail for large num_pixel * num_msaa_samples, so check the error
	// here instead of letting GLCall treat it as a programming error.
	GLClearError();
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, multisample ? static_cast<GLsizei>(num_msaa_samples) : 0,
		GL_DEPTH_COMPONENT24, static_cast<GLsizei>(width), static_cast<GLsizei>(heigth));
	const GLenum alloc_error = glGetError();
	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_DepthBufferID));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));

	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	m_complete = alloc_error == GL_NO_ERROR && status == GL_FRAMEBUFFER_COMPLETE;
	if (!m_complete) {
		SPDLOG_WARN("Framebuffer {}x{} with {} sample(s) not complete (GL error {}, status {})",
			width, heigth, num_msaa_samples, alloc_error, status);
	}
};

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
	GLuint clearColor[4] = { 0, 0, 0, 0 };
	GLCall(glClearBufferuiv(GL_COLOR, 0, clearColor));
	GLCall(glClear(GL_DEPTH_BUFFER_BIT));
}

void FrameBuffer::ClearFloat() const
{
	const GLfloat clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	for (unsigned int i = 0; i < m_NumColorAttachments; ++i) {
		GLCall(glClearBufferfv(GL_COLOR, static_cast<GLint>(i), clearColor));
	}
	GLCall(glClear(GL_DEPTH_BUFFER_BIT));
}

void FrameBuffer::ClearDepth() const
{
	GLCall(glDepthMask(GL_TRUE)); // glClear honours the depth write mask
	GLCall(glClear(GL_DEPTH_BUFFER_BIT));
}

} // namespace vat::gl
