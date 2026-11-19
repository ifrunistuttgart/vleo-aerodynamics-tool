#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "Igeometry_shading_data.h"
#include "Iprojected_area.h"
#include "projected_area_gsi_model.h"

namespace vat::loads {
class PixelForceTorqueCalculator;
}

namespace vat::projected_area {

/**
 * Projected area on the GPU: PixelForceTorqueCalculator run with the ProjectedArea GSI model,
 * whose force per projected area is (1, 0, 0), so the x component of the total force is the
 * area the geometry shows along the direction.
 *
 * The direction follows the convention of v_rel in the load calculators: surfaces whose normal
 * points along it (dot(n, direction) > 0) count, and whatever lies in front of them along the
 * direction shadows them. With MSAA, pixels cut by an edge count with their covered fraction.
 *
 * Owns a hidden OpenGL context. The geometry is referenced; the caller keeps it alive.
 */
class PixelProjectedArea : public IProjectedArea {
public:
    /**
     * @param geometry Geometry to measure.
     * @param num_pixel Edge length of the square render target; the accuracy/runtime knob.
     * @param num_msaa_samples Samples per pixel; 1 = no anti-aliasing.
     * @throws std::invalid_argument if num_pixel or num_msaa_samples is 0.
     */
    PixelProjectedArea(IGeometryShadingData& geometry, unsigned int num_pixel, unsigned int num_msaa_samples = 1);
    ~PixelProjectedArea() override;

    PixelProjectedArea(const PixelProjectedArea&) = delete;
    PixelProjectedArea& operator=(const PixelProjectedArea&) = delete;

    /**
     * @return Projected area along direction [m^2].
     * @throws std::invalid_argument if direction is zero or not finite.
     * @throws std::runtime_error if the GPU evaluation fails.
     */
    [[nodiscard]] float compute_projected_area(const glm::vec3& direction) const override;

private:
    ProjectedArea m_gsi_model; // referenced by m_calculator, so declared before it
    std::unique_ptr<loads::PixelForceTorqueCalculator> m_calculator;
};

} // namespace vat::projected_area
