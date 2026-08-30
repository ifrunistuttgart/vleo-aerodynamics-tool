#pragma once
#include <string>

class Texture2D {
private:
    unsigned int m_texture_id;
    int m_width;
    int m_height;
    unsigned int m_internal_format;
    unsigned int m_format;
    unsigned int m_dtype;
public:
    Texture2D(int width, int height, unsigned int internal_format, unsigned int format, unsigned int dtype);
    ~Texture2D();
    void bind() const;
    void unbind() const;
    void plot_texture(std::string title) const;
    [[nodiscard]] unsigned int get_texture_id() const;
    [[nodiscard]] int get_width() const;
    [[nodiscard]] int get_height() const;
    [[nodiscard]] unsigned int get_internal_format() const;
    [[nodiscard]] unsigned int get_format() const;
    [[nodiscard]] unsigned int get_dtype() const;
};