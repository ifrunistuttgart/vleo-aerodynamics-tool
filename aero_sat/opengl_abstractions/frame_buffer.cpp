#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include "frame_buffer.h"
#include "gl_helpers.h"

FrameBuffer::FrameBuffer(unsigned int width, unsigned int heigth, Texture2D *texture_2d)
	: m_frame_buffer_id(0), m_depth_buffer_id(0), m_color_attachment_counter(0)
{
	//initialize Framebuffer
	GLCall(glGenFramebuffers(1, &m_frame_buffer_id));
	bind();

	attach_texture_2d(texture_2d);

	//attach Depthbuffer
	GLCall(glGenRenderbuffers(1, &m_depth_buffer_id));
	GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_depth_buffer_id));
	GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, heigth));
	GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_buffer_id));

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		SPDLOG_WARN("Framebuffer not complete, remember to attach a texture (status={})", status);
	}
};

FrameBuffer::~FrameBuffer()
{
	GLCall(glDeleteFramebuffers(1, &m_frame_buffer_id));
	GLCall(glDeleteRenderbuffers(1, &m_depth_buffer_id));
};

void FrameBuffer::bind() const
{
	GLint currentFbo = 0;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFbo);

	if (currentFbo == m_frame_buffer_id){
		SPDLOG_DEBUG("Framebuffer {} already bound, skipping bind", m_frame_buffer_id);
		return;
	}

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		SPDLOG_WARN("Binding incomplete Framebuffer (status={})", status);
	}
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer_id));
};

void FrameBuffer::unbind() const
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void FrameBuffer::clear() const
{
	bind();
	SPDLOG_DEBUG("Clearing framebuffer {}",m_frame_buffer_id);

	for (unsigned int attachment = 0; attachment < m_textures.size(); ++attachment) {
		const Texture2D* texture = m_textures[attachment];
		if (texture->get_format() == GL_RED_INTEGER ||
			texture->get_format() == GL_RG_INTEGER ||
			texture->get_format() == GL_RGB_INTEGER ||
			texture->get_format() == GL_RGBA_INTEGER) {
			if (texture->get_dtype() == GL_UNSIGNED_BYTE ||
				texture->get_dtype() == GL_UNSIGNED_SHORT ||
				texture->get_dtype() == GL_UNSIGNED_INT) {
				const GLuint clearColor[4] = { 0, 0, 0, 0 };
				GLCall(glClearBufferuiv(GL_COLOR, attachment, clearColor));
			} else {
				const GLint clearColor[4] = { 0, 0, 0, 0 };
				GLCall(glClearBufferiv(GL_COLOR, attachment, clearColor));
			}
		} else {
			const GLfloat clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			GLCall(glClearBufferfv(GL_COLOR, attachment, clearColor));
		}
	}

	GLCall(glClear(GL_DEPTH_BUFFER_BIT));
}

void FrameBuffer::attach_texture_2d(Texture2D *texture2D)
{
	if (m_textures.size() >= 16) {
		SPDLOG_WARN("Max number of color attachments reached");
		return;
	}

	bind();

	// Attach texture to current slot index
	GLenum attachment_slot = static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + m_textures.size());
	GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment_slot, GL_TEXTURE_2D, texture2D->get_texture_id(), 0));
	m_textures.push_back(texture2D);

	// Automatically update active draw buffers for MRT
	std::vector<GLenum> attachments(m_textures.size());
	for (std::size_t i = 0; i < m_textures.size(); ++i) {
		attachments[i] = static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i);
	}
	GLCall(glDrawBuffers(static_cast<GLsizei>(attachments.size()), attachments.data()));
}

const Texture2D* FrameBuffer::get_texture(std::size_t index) const
{
	if (index >= m_textures.size()) {
		SPDLOG_WARN("Framebuffer texture index {} is out of range (size={})", index, m_textures.size());
		return nullptr;
	}
	return m_textures[index];
}

std::size_t FrameBuffer::get_color_attachment_count() const
{
	return m_textures.size();
}