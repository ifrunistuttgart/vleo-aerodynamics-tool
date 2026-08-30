#pragma once
#include <GL/glew.h>

class ShaderStorageBuffer {
private:
    unsigned int m_shader_storage_buffer_id;
public:
    ShaderStorageBuffer(const void* data, size_t size);
    ~ShaderStorageBuffer();
    void bind();
    void unbind();
    void bind_base(unsigned int binding_point);
    void get_data(void* data, size_t size);
};