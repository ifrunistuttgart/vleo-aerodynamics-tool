#pragma once
#include <span>

namespace vat::gl {

class FrameBuffer
{
private:
	unsigned int m_FrameBufferID;
	unsigned int m_DepthBufferID;
	unsigned int m_NumColorAttachments;
public:
	FrameBuffer(unsigned int texture2D, unsigned int width, unsigned int heigth);
	// One colour attachment per texture, in order, all of them draw buffers.
	FrameBuffer(std::span<const unsigned int> colorTextures, unsigned int width, unsigned int heigth);
	~FrameBuffer();

	void Bind() const;
	void UnBind() const;
	// Clears colour attachment 0 of an integer target, and the depth buffer.
	void Clear() const;
	// Clears every colour attachment of a float target to zero, and the depth buffer.
	void ClearFloat() const;
};

} // namespace vat::gl
