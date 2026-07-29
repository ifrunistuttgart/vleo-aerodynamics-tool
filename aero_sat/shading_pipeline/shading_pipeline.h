#pragma once
#include <memory>
#include <span>
#include <glm/glm.hpp>
#include "Ishading_pipeline.h"
#include "Isatellite_shading_data.h"
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
    std::vector<float> shade(const glm::vec3& v_rel_hat) override;
};