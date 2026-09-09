#pragma once
#include "Igsi_model_gpu.h"
#include <string>
namespace gsi::gpu {
    class Newton: public IGSIModelGPU{
    public:
        Newton(){};
        ~Newton() override = default;

        [[nodiscard]] std::string get_vertex_shader_code() override;
        void set_shader_uniforms(Shader* shader) override;
        void set_gsi_parameter(std::string name, float value) override;
        [[nodiscard]] float get_gsi_parameter(std::string name) const override;
    private:
        std::string m_vertex_shader = R"GLSL(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec3 aNormal;

            uniform mat4 model;
            uniform mat4 view;
            uniform mat4 projection;
            uniform mat3 normalMatrix;

            uniform float aero_pressure; // 0.5*rho*v^2
            uniform vec3 windDir;

            flat out vec3 pressureVec;
            out vec3 FragPos;
            flat out float cos_d;

            void main()
            {
                vec4 worldPos = model * vec4(aPos, 1.0);
                FragPos = worldPos.xyz;

                vec3 Normal = normalMatrix * aNormal;

                vec3 nNormal = dot(Normal, Normal) > 0.0 ? normalize(Normal) : vec3(0.0);
                vec3 nWindDir = normalize(windDir);
                cos_d = dot(nWindDir,nNormal);
                if (cos_d > 0){
                    float cp = 2.0 * cos_d * cos_d;
                    pressureVec = -cp * aero_pressure * nNormal;
                } else {
                    pressureVec = vec3(0.0);
                }
                gl_Position = projection * view * worldPos;
            }
            )GLSL";
    };
}