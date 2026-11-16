#pragma once
#include <memory>
#include <span>
#include <vector>

#include "texture_2d.h"

namespace vat::gl {

class FrameBuffer
{
private:
	unsigned int m_FrameBufferID = 0;
	unsigned int m_DepthBufferID = 0;
	unsigned int m_NumColorAttachments = 0;
	// Shared with the caller, so an attached texture lives at least as long as the framebuffer.
	std::vector<std::shared_ptr<Texture2D>> m_color_textures;
	unsigned int m_width;
	unsigned int m_height;
	unsigned int m_num_msaa_samples;
	bool m_depth_allocated = false;
	bool m_complete = false;

	void check_completeness();
public:
	// Creates the framebuffer with a depth buffer and no colour attachment (a depth-only
	// framebuffer). With `num_msaa_samples` > 1 the depth buffer is multisampled (MSAA);
	// 1 is an ordinary framebuffer. Leaves the framebuffer bound.
	FrameBuffer(unsigned int width, unsigned int heigth, unsigned int num_msaa_samples = 1);
	~FrameBuffer();

	FrameBuffer(const FrameBuffer&) = delete;
	FrameBuffer& operator=(const FrameBuffer&) = delete;

	// One colour attachment per texture, in order, all of them draw buffers. Replaces earlier
	// attachments. The framebuffer keeps the textures alive. Every texture must be non-null,
	// valid and have the framebuffer's size and sample count, otherwise std::invalid_argument
	// is thrown and nothing changes. Leaves the framebuffer bound; is_complete() is updated.
	void attach_color_textures(std::span<const std::shared_ptr<Texture2D>> colorTextures);

	// False if the depth buffer could not be allocated (e.g. out of memory) or the
	// framebuffer is incomplete.
	bool is_complete() const { return m_complete; }

	void Bind() const;
	void UnBind() const;
	// Clears every attached colour texture to zero (each through Texture2D::clear(), so integer
	// and float formats are both handled) and then the depth buffer. The framebuffer must be bound.
	void Clear() const;
	// Clears the depth buffer only.
	void ClearDepth() const;
	// Resolves every colour attachment of this (multisampled) framebuffer into the same attachment
	// of `target` (single-sample, same size and attachment count) with glBlitFramebuffer. How the
	// samples are combined is up to the driver; GL only says they are typically averaged.
	// Leaves no framebuffer bound. Throws std::invalid_argument on a size or attachment mismatch.
	void resolve_color_into(const FrameBuffer& target) const;
	[[nodiscard]] unsigned int get_width() const { return m_width; }
	[[nodiscard]] unsigned int get_height() const { return m_height; }
	[[nodiscard]] unsigned int get_num_msaa_samples() const { return m_num_msaa_samples; }
};

} // namespace vat::gl
