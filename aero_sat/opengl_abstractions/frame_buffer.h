#pragma once
#include <vector>
#include "texture_2d.h"
class FrameBuffer
{
private:
	unsigned int m_frame_buffer_id;
	unsigned int m_depth_buffer_id;
	unsigned int m_color_attachment_counter;
	std::vector<const Texture2D*> m_textures;
public:
	FrameBuffer(unsigned int width, unsigned int heigth, Texture2D *texture_2d);
	~FrameBuffer();

	void bind() const;
	void unbind() const;
	void clear() const;
	void attach_texture_2d(Texture2D* texture2D);
	[[nodiscard]] const Texture2D* get_texture(std::size_t index) const;
	[[nodiscard]] std::size_t get_color_attachment_count() const;
};
