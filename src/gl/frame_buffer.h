#pragma once
#include <span>

namespace vat::gl {

class FrameBuffer
{
private:
	unsigned int m_FrameBufferID = 0;
	unsigned int m_DepthBufferID = 0;
	unsigned int m_NumColorAttachments = 0;
	bool m_complete = false;
public:
	FrameBuffer(unsigned int texture2D, unsigned int width, unsigned int heigth);
	// One colour attachment per texture, in order, all of them draw buffers, plus a
	// depth buffer. With `num_msaa_samples` > 1 the depth buffer is multisampled (MSAA)
	// and the colour textures must be GL_TEXTURE_2D_MULTISAMPLE textures with the same
	// sample count; num_msaa_samples = 1 is an ordinary framebuffer. An empty span gives a
	// depth-only framebuffer (no colour attachment).
	FrameBuffer(std::span<const unsigned int> colorTextures, unsigned int width, unsigned int heigth,
		unsigned int num_msaa_samples = 1);
	~FrameBuffer();

	FrameBuffer(const FrameBuffer&) = delete;
	FrameBuffer& operator=(const FrameBuffer&) = delete;

	// False if the depth buffer could not be allocated (e.g. out of memory) or the
	// framebuffer is incomplete.
	bool is_complete() const { return m_complete; }

	void Bind() const;
	void UnBind() const;
	// Clears colour attachment 0 of an integer target, and the depth buffer.
	void Clear() const;
	// Clears every colour attachment of a float target to zero, and the depth buffer.
	void ClearFloat() const;
	// Clears the depth buffer only.
	void ClearDepth() const;
};

} // namespace vat::gl
