#include "newton.h"
#include <spdlog/spdlog.h>
namespace gsi::gpu {
    std::string Newton::get_vertex_shader_code() {
        return m_vertex_shader;
    }

    void Newton::set_shader_uniforms(Shader *shader) {
        // No additional uniforms needed for Newton model
    }

    void Newton::set_gsi_parameter(std::string name, float value) {
        SPDLOG_WARN("GPU Gsi model Newton has no gsi parameter {}", name);
    }

    float Newton::get_gsi_parameter(std::string name) const {
        SPDLOG_WARN("GPU Gsi model Newton has no gsi parameter {} returning 0", name);
        return 0.0f;
    }
}
