#pragma once
#include <memory>
#include <span>
#include <vector>

#include "texture_2d.h"

namespace vat::gl {

/**
 * @brief Framebuffer object with a depth renderbuffer and optional colour textures.
 *
 * Non-copyable. A context must be current when the framebuffer is created, used and destroyed.
 */
class FrameBuffer {
private:
    unsigned int m_frame_buffer_id = 0;
    unsigned int m_depth_buffer_id = 0;
    unsigned int m_num_color_attachments = 0;
    // Shared with the caller, so an attached texture lives at least as long as the framebuffer.
    std::vector<std::shared_ptr<Texture2D>> m_color_textures;
    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_num_msaa_samples;
    bool m_depth_allocated = false;
    bool m_complete = false;

    void check_completeness();

public:
    /**
     * @brief Creates the framebuffer with a depth buffer and no colour attachment (a depth-only
     * framebuffer).
     *
     * Leaves the framebuffer bound. Check is_complete(): allocation may fail without throwing.
     *
     * @param width Width in pixels.
     * @param heigth Height in pixels.
     * @param num_msaa_samples With > 1 the depth buffer is multisampled (MSAA); 1 is an ordinary
     *        framebuffer.
     */
    FrameBuffer(unsigned int width, unsigned int heigth, unsigned int num_msaa_samples = 1);
    ~FrameBuffer();

    FrameBuffer(const FrameBuffer&) = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;

    /**
     * @brief Attaches one colour attachment per texture, in order, all of them draw buffers.
     *
     * Replaces earlier attachments. The framebuffer keeps the textures alive. Leaves the
     * framebuffer bound; is_complete() is updated.
     *
     * @param color_textures Textures to attach; each must be non-null, valid and have the
     *        framebuffer's size and sample count.
     * @throws std::invalid_argument if a texture violates that; nothing changes then.
     */
    void attach_color_textures(std::span<const std::shared_ptr<Texture2D>> color_textures);

    /**
     * @brief False if the depth buffer could not be allocated (e.g. out of memory) or the
     * framebuffer is incomplete.
     */
    bool is_complete() const { return m_complete; }

    /** @brief Binds the framebuffer as GL_FRAMEBUFFER. */
    void bind() const;
    /** @brief Binds the default framebuffer. */
    void unbind() const;
    /**
     * @brief Clears every attached colour texture to zero, then the depth buffer.
     *
     * Each texture goes through Texture2D::clear(), so integer and float formats are both
     * handled. The framebuffer must be bound.
     */
    void clear() const;
    /** @brief Clears the depth buffer only. */
    void clear_depth() const;
    /**
     * @brief Resolves every colour attachment of this (multisampled) framebuffer into the same
     * attachment of `target` with glBlitFramebuffer.
     *
     * How the samples are combined is up to the driver; GL only says they are typically
     * averaged. Leaves no framebuffer bound.
     *
     * @param target Single-sample framebuffer of the same size and attachment count.
     * @throws std::invalid_argument on a size or attachment mismatch.
     */
    void resolve_color_into(const FrameBuffer& target) const;
    /** @brief Width in pixels. */
    [[nodiscard]] unsigned int get_width() const { return m_width; }
    /** @brief Height in pixels. */
    [[nodiscard]] unsigned int get_height() const { return m_height; }
    /** @brief Samples per pixel of the depth buffer, as requested in the constructor. */
    [[nodiscard]] unsigned int get_num_msaa_samples() const { return m_num_msaa_samples; }
};

} // namespace vat::gl
