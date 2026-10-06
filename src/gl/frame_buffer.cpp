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

FrameBuffer::FrameBuffer(std::span<const unsigned int> colorTextures, unsigned int width, unsigned int heigth)
	: m_NumColorAttachments(static_cast<unsigned int>(colorTextures.size()))
{
	//initialize Framebuffer
	GLCall(glGenFramebuffers(1, &m_FrameBufferID));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID));

	//attach Textures
	std::vector<GLenum> drawBuffers;
	for (unsigned int i = 0; i < m_NumColorAttachments; ++i) {
		GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorTextures[i], 0));
		drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
	}
	GLCall(glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data()));

	//attach Depthbuffer
	GLCall(glGenRenderbuffers(1, &m_DepthBufferID));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBufferID));
	GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, heigth));
	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_DepthBufferID));

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		SPDLOG_WARN("Framebuffer not complete (status={})", status);
	}
};

FrameBuffer::~FrameBuffer()
{
	GLCall(glDeleteFramebuffers(1, &m_FrameBufferID));
	GLCall(glDeleteRenderbuffers(1, &m_DepthBufferID));
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

} // namespace vat::gl
