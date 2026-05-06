#pragma once
#include <memory>
#include <span>
#include <glm/glm.hpp>
#include "Ishading_pipeline.h"
#include "Core/ISatellite_shading_data.h"
#include "src/Ishading_algorithm.h"
#include "src/shading_algorithm_factory.h"
#include "src/opengl/glfw_opengl_context.h"

class ShadingPipeline : public IShadingPipeline {
private:
    std::unique_ptr<GlfwOpenGLContext> m_context;      // zuerst deklariert
    std::unique_ptr<IShadingAlgorithm> m_algorithm;    // wird zuerst zerstört
    ISatelliteShadingData& m_satellite;

public:
    ShadingPipeline(
        ISatelliteShadingData& satellite,
        ShadingAlgorithmType algorithm_type,
        unsigned int num_pixel);

    ~ShadingPipeline() override;
    int shade(std::span<float> triangle_visibility, const glm::vec3& v_rel_hat) override;
};