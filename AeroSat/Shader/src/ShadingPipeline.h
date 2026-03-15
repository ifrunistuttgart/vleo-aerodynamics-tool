#pragma once
#include <memory>
#include <eigen3/Eigen/Dense>
#include "Aero_Force_Torque/IShadingPipeline.h"
#include "Core/ISatellite_shading_data.h"
#include "src/IShading_Algorithm.h"
#include "src/ShadingAlgorithmFactory.h"
#include "src/opengl/GlfwOpenGLContext.h"

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
    int shade(float* triangle_visibility, const Eigen::Vector3f& v_rel_hat) override;
};