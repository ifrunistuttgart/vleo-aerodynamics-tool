#pragma once
#include <vector>

namespace vat::gl {

enum class TextureFilter { Nearest, Linear };

/**
 * RAII wrapper of a 2D texture with immutable storage, single-sample (GL_TEXTURE_2D) or
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

    /// False if the storage could not be allocated (e.g. out of memory).
    [[nodiscard]] bool is_valid() const { return m_texture_id != 0; }

    /// Binds to `unit` (0 = GL_TEXTURE0) on get_target(); the active unit is GL_TEXTURE0 afterwards.
    void bind(unsigned int unit = 0) const;
    void unbind(unsigned int unit = 0) const;
    /// Binds level 0 to image unit `unit`; `access` is GL_READ_ONLY, GL_WRITE_ONLY or GL_READ_WRITE.
    void bind_as_image(unsigned int unit, unsigned int access) const;

    /**
     * Sets every texel (and every sample of a multisample texture) of level 0 to zero, whatever
     * the format: integer formats are cleared as integers, the others as floats. Independent of
     * the current framebuffer binding.
     */
    void clear() const;

    /**
     * Reads level 0 back as floats, row 0 at the bottom, with as many channels as `format` has.
     * Only for a single-sample texture; throws std::logic_error on a multisample texture.
     */
    [[nodiscard]] std::vector<float> read_back_float() const;

    [[nodiscard]] unsigned int get_texture_id() const { return m_texture_id; }
    [[nodiscard]] unsigned int get_target() const;
    [[nodiscard]] bool is_multisample() const { return m_num_samples > 1; }
    [[nodiscard]] int get_num_samples() const { return m_num_samples; }
    [[nodiscard]] int get_width() const { return m_width; }
    [[nodiscard]] int get_height() const { return m_height; }
    [[nodiscard]] unsigned int get_internal_format() const { return m_internal_format; }
    [[nodiscard]] unsigned int get_format() const { return m_format; }
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
