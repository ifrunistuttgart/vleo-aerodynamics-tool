#pragma once
#include "Iaero_load_calculator.h"
#include "Isatellite_shading_data.h"
#include "frame_buffer.h"
#include "vertex_array.h"
#include "shader.h"
#include "compute_shader.h"
#include "glfw_opengl_context.h"
#include <memory>
class GPUAeroLoadCalculator: public IAeroLoadCalculator {
public:
    GPUAeroLoadCalculator(ISatelliteShadingData& satellite, int num_pixel);
    ~GPUAeroLoadCalculator() override = default;
    int calc_aero_torque_force(const glm::vec3 &v_rel__m_per_s, float surface_temp__K, AeroConditions &aero, glm::vec3 &torque__Nm, glm::vec3 &force__N) override;

private:
    const unsigned int m_num_pixel;
    std::unique_ptr<Texture2D> m_position_texture;
    std::unique_ptr<Texture2D> m_normal_texture;
    std::unique_ptr<Texture2D> m_float_texture;
    std::unique_ptr<FrameBuffer> m_frame_buffer;
    std::unique_ptr<VertexArray> m_vertex_array;
    std::unique_ptr<VertexBuffer> m_vertex_buffer;
    std::unique_ptr<Shader> m_shader;
    std::unique_ptr<ComputeShader> m_compute_shader;
    ISatelliteShadingData& m_satellite;
    std::unique_ptr<GlfwOpenGLContext> m_context;

};