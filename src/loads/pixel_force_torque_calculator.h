#pragma once
#include <memory>
#include <vector>
#include "Iaero_load_calculator.h"
#include "Igeometry_shading_data.h"
#include "Igsi_model.h"
#include "core.h"

namespace vat::loads {

struct PixelLoadOptions {
    /**
     * Lower bound for cos(delta) in the wetted area A_px / cos(delta) of a pixel.
     * Models whose force per projected area diverges at grazing incidence clamp with
     * it too. Only affects surfaces seen almost edge-on.
     */
    float min_cos_delta = 1e-3f;

    /** Read back the per-pixel pressure after every evaluation, see pressure_image(). */
    bool keep_pressure_image = false;
};

/**
 * Integrates force and torque per pixel on the GPU, so the result depends on the
 * render resolution only and not on how the geometry is meshed.
 *
 * Pass 1 renders the body-frame position and normal of the front-most surface in
 * every pixel of an orthographic view along the flow. Pass 2 evaluates the GSI model
 * in every windward pixel, dF = f(n, v) * A_px with f the force per projected area and
 * A_px = (2R / num_pixel)^2, dT = r x dF, and sums both on the GPU. Surfaces facing
 * away from the flow (dot(n, v) <= 0) are invisible to the camera; they are evaluated
 * per triangle on the CPU with the GSI model and no shadowing, as in
 * HybridForceTorqueCalculator.
 *
 * Owns its own hidden OpenGL context, so it needs no shading pipeline. The GSI model
 * must provide a GPU implementation (IGSIModel::glsl_force_per_projected_area()).
 */
class PixelForceTorqueCalculator : public IAeroLoadCalculator {
public:
    /**
     * @param geometry Reference to the geometry shading data.
     * @param gsi_model Reference to the GSI model; must have a GPU implementation.
     * @param num_pixel Edge length of the square render target.
     * @param options See PixelLoadOptions.
     * @throws std::invalid_argument if the model has no GPU implementation or num_pixel is 0.
     */
    PixelForceTorqueCalculator(IGeometryShadingData& geometry, IGSIModel& gsi_model, unsigned int num_pixel, PixelLoadOptions options = {});
    ~PixelForceTorqueCalculator() override;

    PixelForceTorqueCalculator(const PixelForceTorqueCalculator&) = delete;
    PixelForceTorqueCalculator& operator=(const PixelForceTorqueCalculator&) = delete;

    int calc_aero_torque_force(const glm::vec3& v_rel__m_per_s, float surface_temp__K, AeroConditions& aero, glm::vec3& torque__Nm, glm::vec3& force__N) override;

    /**
     * Pressure (normal component of the force per wetted area) [N/m^2] of the last
     * evaluation, num_pixel x num_pixel, row-major with row 0 at the bottom of the
     * flow view. Zero outside windward surfaces. Empty unless keep_pressure_image is set.
     */
    [[nodiscard]] const std::vector<float>& pressure_image() const { return m_pressure_image; }

    /** Wetted area of the windward surfaces the flow reaches, from the last evaluation [m^2]. */
    [[nodiscard]] double last_wetted_area() const { return m_wetted_area__m2; }

    /** Area normal to the flow covered by windward surfaces, from the last evaluation [m^2]. */
    [[nodiscard]] double last_projected_area() const { return m_projected_area__m2; }

private:
    struct GpuState;

    IGeometryShadingData& m_geometry;
    IGSIModel& m_gsi_model;
    const unsigned int m_num_pixel;
    const PixelLoadOptions m_options;
    std::unique_ptr<GpuState> m_gpu;

    std::vector<float> m_pressure_image;
    double m_wetted_area__m2 = 0.0;
    double m_projected_area__m2 = 0.0;
};

} // namespace vat::loads
