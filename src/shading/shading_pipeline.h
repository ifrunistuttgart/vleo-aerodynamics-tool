#pragma once
#include <memory>
#include <span>
#include <glm/glm.hpp>
#include "Ishading_pipeline.h"
#include "Igeometry_shading_data.h"
#include "Ishading_algorithm.h"
#include "shading_algorithm_factory.h"
#include "glfw_opengl_context.h"
#include "pixel_sizing.h"

namespace vat::shading {

class ShadingPipeline : public IShadingPipeline {
private:
    std::unique_ptr<gl::GlfwOpenGLContext> m_context;
    std::unique_ptr<IShadingAlgorithm> m_algorithm;
    IGeometryShadingData& m_geometry;
    unsigned int m_num_pixel;
    // Radius at construction, when num_pixel was fixed; see shade().
    float m_initial_radius__m;
    bool m_warned_radius_growth = false;

public:
    /**
     * @param num_pixel Edge length of the square render target. The frustum spans the
     *                  bounding sphere, so one pixel is 2R / num_pixel wide.
     * @throws std::invalid_argument if num_pixel is 0 or exceeds what the GPU supports.
     */
    ShadingPipeline(
        IGeometryShadingData& geometry,
        ShadingAlgorithmType algorithm_type,
        unsigned int num_pixel
    );

    /**
     * As above, with num_pixel chosen from the mesh by suggest_num_pixel(). Build it with
     * the geometry in its most extended pose: num_pixel is fixed here, and a pose that
     * later reaches further out spreads the same pixels over a larger radius.
     */
    ShadingPipeline(
        IGeometryShadingData& geometry,
        ShadingAlgorithmType algorithm_type
    );

    unsigned int get_num_pixel() const { return m_num_pixel; }

    ~ShadingPipeline() override;
    std::vector<float> shade(const glm::vec3& v_rel_hat) override;
};

} // namespace vat::shading
