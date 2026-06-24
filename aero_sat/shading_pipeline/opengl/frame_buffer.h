#pragma once

class FrameBuffer
{
private:
	unsigned int m_FrameBufferID;
	unsigned int m_DepthBufferID;
public:
	FrameBuffer(unsigned int texture2D, unsigned int width, unsigned int heigth);
	~FrameBuffer();

	void Bind() const;
	void UnBind() const;
	void Clear() const;
};
