#include "pixel_projected_area.h"

#include <cmath>
#include <stdexcept>
#include "core.h"
#include "pixel_force_torque_calculator.h"

namespace vat::projected_area {

PixelProjectedArea::PixelProjectedArea(IGeometryShadingData& geometry, unsigned int num_pixel,
                                       unsigned int num_msaa_samples)
    // Leeward surfaces add nothing to the projected area, so the Hyperthermal regime skips the
    // CPU loop over them.
    : m_calculator(std::make_unique<loads::PixelForceTorqueCalculator>(
          geometry, m_gsi_model, num_pixel, num_msaa_samples, false, 1e-3f, FlowRegime::Hyperthermal)) {}

PixelProjectedArea::~PixelProjectedArea() = default;

float PixelProjectedArea::compute_projected_area(const glm::vec3& direction) const {
    const float length = glm::length(direction);
    if (!std::isfinite(length) || length == 0.0f) {
        throw std::invalid_argument("PixelProjectedArea: direction must be finite and non-zero");
    }

    // The model ignores speed, surface temperature and atmosphere.
    AeroConditions aero{};
    glm::vec3 force__N(0.0f);
    glm::vec3 torque__Nm(0.0f);
    if (m_calculator->calc_aero_torque_force(direction / length, 300.0f, aero, torque__Nm, force__N) != 0) {
        throw std::runtime_error("PixelProjectedArea: the GPU evaluation failed");
    }
    return force__N.x;
}

} // namespace vat::projected_area
