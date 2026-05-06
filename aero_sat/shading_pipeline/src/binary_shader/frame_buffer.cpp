#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include "frame_buffer.h"
#include "src/opengl/gl_helpers.h"


FrameBuffer::FrameBuffer(unsigned int texture2D, unsigned int width, unsigned int heigth)
{
	//initialize Framebuffer
	GLCall(glGenFramebuffers(1, &m_FrameBufferID));
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID));

	//attach Texture
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture2D, 0));

	//attach Depthbuffer
	GLCall(glGenRenderbuffers(1, &m_DepthBufferID));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBufferID));
	GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, heigth));
	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_DepthBufferID));

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		spdlog::warn("Framebuffer not complete! Status: {}", status);
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