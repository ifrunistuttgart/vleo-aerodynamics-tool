#pragma once
#include <vector>

namespace vat::gl {

/** @brief Min/mag filter of a single-sample Texture2D. */
enum class TextureFilter { Nearest, Linear };

/**
 * @brief RAII wrapper of a 2D texture with immutable storage, single-sample (GL_TEXTURE_2D) or
 * multisampled (GL_TEXTURE_2D_MULTISAMPLE).
 *
 * `num_samples` > 1 selects the multisample target. A multisample texture has fixed sample
 * locations, takes no sampler state and cannot be read back with read_back_float().
 *
 * Allocation can legitimately fail for a large size times sample count, so the constructor does
 * not treat a GL error as a programming error. Check is_valid() before using the texture; an
 * invalid texture owns no GL name.
 *
 * Move-only. A context must be current when the texture is created and destroyed.
 * The GL enums are plain unsigned ints so this header does not depend on GLEW.
 */
class Texture2D {
public:
    /**
     * @brief Allocates immutable storage for level 0; check is_valid() afterwards.
     *
     * @param width Width in texels.
     * @param height Height in texels.
     * @param internal_format Sized internal format, e.g. GL_RGBA32F.
     * @param format Pixel format used for read-back, e.g. GL_RED.
     * @param dtype Component type, e.g. GL_FLOAT.
     * @param num_samples Samples per texel; 1 gives an ordinary GL_TEXTURE_2D.
     * @param filter Min/mag filter of a single-sample texture; ignored when multisampled.
     */
    Texture2D(int width, int height, unsigned int internal_format, unsigned int format, unsigned int dtype,
              int num_samples = 1, TextureFilter filter = TextureFilter::Nearest);
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    /** @brief False if the storage could not be allocated (e.g. out of memory). */
    [[nodiscard]] bool is_valid() const { return m_texture_id != 0; }

    /**
     * @brief Binds to `unit` (0 = GL_TEXTURE0) on get_target(); the active unit is GL_TEXTURE0
     * afterwards.
     * @param unit Texture unit index.
     */
    void bind(unsigned int unit = 0) const;
    /**
     * @brief Unbinds get_target() from `unit`.
     * @param unit Texture unit index.
     */
    void unbind(unsigned int unit = 0) const;
    /**
     * @brief Binds level 0 to image unit `unit`.
     * @param unit Image unit index.
     * @param access GL_READ_ONLY, GL_WRITE_ONLY or GL_READ_WRITE.
     */
    void bind_as_image(unsigned int unit, unsigned int access) const;

    /**
     * @brief Sets every texel (and every sample of a multisample texture) of level 0 to zero, whatever
     * the format: integer formats are cleared as integers, the others as floats. Independent of
     * the current framebuffer binding.
     */
    void clear() const;

    /**
     * @brief Reads level 0 back as floats, row 0 at the bottom, with as many channels as `format` has.
     *
     * Only for a single-sample texture.
     *
     * @return width * height * channels floats, row-major.
     * @throws std::logic_error on a multisample texture or an unsupported pixel format.
     */
    [[nodiscard]] std::vector<float> read_back_float() const;

    /** @brief GL texture name; 0 if invalid or moved from. */
    [[nodiscard]] unsigned int get_texture_id() const { return m_texture_id; }
    /** @brief GL_TEXTURE_2D or GL_TEXTURE_2D_MULTISAMPLE. */
    [[nodiscard]] unsigned int get_target() const;
    /** @brief Whether the texture has more than one sample per texel. */
    [[nodiscard]] bool is_multisample() const { return m_num_samples > 1; }
    /** @brief Samples per texel. */
    [[nodiscard]] int get_num_samples() const { return m_num_samples; }
    /** @brief Width in texels. */
    [[nodiscard]] int get_width() const { return m_width; }
    /** @brief Height in texels. */
    [[nodiscard]] int get_height() const { return m_height; }
    /** @brief Sized internal format, e.g. GL_RGBA32F. */
    [[nodiscard]] unsigned int get_internal_format() const { return m_internal_format; }
    /** @brief Pixel format used for read-back. */
    [[nodiscard]] unsigned int get_format() const { return m_format; }
    /** @brief Component type used for read-back. */
    [[nodiscard]] unsigned int get_dtype() const { return m_dtype; }

private:
    unsigned int m_texture_id = 0;
    int m_width;
    int m_height;
    unsigned int m_internal_format;
    unsigned int m_format;
    unsigned int m_dtype;
    int m_num_samples;
};

} // namespace vat::gl
