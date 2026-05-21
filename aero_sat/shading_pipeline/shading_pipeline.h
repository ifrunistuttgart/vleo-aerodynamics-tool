#pragma once
#include <memory>
#include <span>
#include <glm/glm.hpp>
#include "Ishading_pipeline.h"
#include "ISatellite_shading_data.h"
#include "Ishading_algorithm.h"
#include "shading_algorithm_factory.h"
#include "opengl/glfw_opengl_context.h"

class ShadingPipeline : public IShadingPipeline {
private:
    std::unique_ptr<GlfwOpenGLContext> m_context;
    std::unique_ptr<IShadingAlgorithm> m_algorithm;
    ISatelliteShadingData& m_satellite;

public:
    ShadingPipeline(
        ISatelliteShadingData& satellite,
        ShadingAlgorithmType algorithm_type,
        unsigned int num_pixel
    );

    ~ShadingPipeline() override;
    int shade(std::span<float> triangle_visibility, const glm::vec3& v_rel_hat) override;
};