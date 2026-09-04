#pragma once
#include <GL/glew.h>

class ShaderStorageBuffer {
private:
    unsigned int m_shader_storage_buffer_id;
    unsigned int m_binding_point;
    unsigned int m_size;
public:
    ShaderStorageBuffer(const void* data, size_t size);
    ~ShaderStorageBuffer();
    void bind();
    void unbind();
    void bind_base(unsigned int binding_point);
    void get_data(void* data, size_t size);
    void set_zero();
};