#pragma once
#include <memory>
#include <span>
#include <glm/glm.hpp>
#include "Ishading_pipeline.h"
#include "Igeometry_shading_data.h"
#include "Ishading_algorithm.h"
#include "shading_algorithm_factory.h"
#include "opengl/glfw_opengl_context.h"

namespace vat {

class ShadingPipeline : public IShadingPipeline {
private:
    std::unique_ptr<gl::GlfwOpenGLContext> m_context;
    std::unique_ptr<IShadingAlgorithm> m_algorithm;
    IGeometryShadingData& m_geometry;

public:
    ShadingPipeline(
        IGeometryShadingData& geometry,
        ShadingAlgorithmType algorithm_type,
        unsigned int num_pixel
    );

    ~ShadingPipeline() override;
    std::vector<float> shade(const glm::vec3& v_rel_hat) override;
};

} // namespace vat
