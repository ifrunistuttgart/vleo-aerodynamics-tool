#pragma once
#include <memory>
#include <optional>
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

    // Marks the constructor that takes an already resolved, non-optional num_pixel.
    struct ResolvedTag {};
    ShadingPipeline(IGeometryShadingData& geometry, ShadingAlgorithmType algorithm_type,
        unsigned int num_pixel, unsigned int num_msaa_samples, ResolvedTag);

public:
    /**
     * @param num_pixel Edge length of the square render target. The frustum spans the
     *                  bounding sphere, so one pixel is 2R / num_pixel wide. When omitted it is
     *                  chosen from the mesh by suggest_num_pixel() (taking num_msaa_samples into
     *                  account); build the pipeline then with the geometry in its most extended
     *                  pose, since num_pixel is fixed here and a pose that later reaches
     *                  further out spreads the same pixels over a larger radius.
     * @param num_msaa_samples Samples per pixel. 1 (default) means no anti-aliasing; > 1 enables
     *                    MSAA, which only the Binary algorithm supports.
     * @throws std::invalid_argument if num_pixel is 0 or exceeds what the GPU supports, or if
     *         num_msaa_samples is 0 or > 1 for an algorithm other than Binary.
     * @throws std::runtime_error if the shading algorithm cannot set up its GPU resources
     *         (for example the geometry has too many triangles, or the render target does
     *         not fit in GPU memory).
     */
    ShadingPipeline(
        IGeometryShadingData& geometry,
        ShadingAlgorithmType algorithm_type,
        std::optional<unsigned int> num_pixel = std::nullopt,
        unsigned int num_msaa_samples = 1
    );

    unsigned int get_num_pixel() const { return m_num_pixel; }

    ~ShadingPipeline() override;
    std::vector<float> shade(const glm::vec3& v_rel_hat) override;
};

} // namespace vat::shading
