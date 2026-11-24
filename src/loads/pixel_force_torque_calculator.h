#pragma once
#include <atomic>
#include <memory>
#include <vector>
#include "Iaero_load_calculator.h"
#include "Igeometry_shading_data.h"
#include "Igsi_model.h"
#include "core.h"

namespace vat::loads {

/**
 * Integrates force and torque per pixel on the GPU, so the result depends on the
 * render resolution only and not on how the geometry is meshed.
 *
 * Pass 1 renders the front-most surface in every pixel of an orthographic view along
 * the flow and evaluates the GSI model once per pixel and triangle with the triangle's
 * exact normal, storing f(n, v) and r x f, with f the force per projected area. Pass 2
 * sums dF = f * A_px and dT = (r x f) * A_px over the windward pixels on the GPU, with
 * A_px = (2R / num_pixel)^2.
 *
 * With MSAA, pass 1 stores the load in every sample the triangle covers; the hardware resolve
 * (glBlitFramebuffer, which averages the samples) then gives each pixel the mean load of
 * its samples. A pixel cut by a silhouette or shadow edge counts with its covered part, and
 * a pixel on a crease gets the mean of its faces' loads, as if every sample had been
 * evaluated.
 *
 * Surfaces facing away from the flow (dot(n, v) <= 0) are invisible to the camera; they
 * are evaluated per triangle on the CPU with the GSI model and no shadowing, as in
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
     * @param num_msaa_samples Samples per pixel (anti-aliasing). 1 = none: a pixel counts fully
     *        if its centre is covered. > 1 = MSAA with that many samples: a partly covered pixel
     *        counts with its covered fraction, so silhouettes and shadow edges are resolved to a
     *        fraction of a pixel. Clamped to what the driver supports (with a warning). The
     *        G-buffer takes about k * 16 B * num_pixel^2 * (num_msaa_samples + 1): the
     *        multisampled targets plus the single-sample copy they are resolved into (none with
     *        1 sample), with k = 2, or 3 with keep_pressure_image.
     * @param keep_pressure_image Read back the per-pixel pressure after every evaluation, see
     *        pressure_image().
     * @param min_cos_delta Lower bound for cos(delta) in the wetted area A_px / cos(delta) of a
     *        pixel. Models whose force per projected area diverges at grazing incidence clamp
     *        with it too. Only affects surfaces seen almost edge-on.
     * @param flow_regime Initial regime; Hyperthermal skips leeward-facing triangles, see
     *        FlowRegime. Change it later with set_flow_regime().
     * @throws std::invalid_argument if the model has no GPU implementation, num_pixel is 0 or
     *         num_msaa_samples is 0.
     * @throws std::runtime_error if the G-buffer cannot be allocated even with one sample.
     */
    PixelForceTorqueCalculator(IGeometryShadingData& geometry, IGSIModel& gsi_model, unsigned int num_pixel,
                               unsigned int num_msaa_samples = 1, bool keep_pressure_image = false,
                               float min_cos_delta = 1e-3f, FlowRegime flow_regime = FlowRegime::Hypothermal);
    ~PixelForceTorqueCalculator() override;

    PixelForceTorqueCalculator(const PixelForceTorqueCalculator&) = delete;
    PixelForceTorqueCalculator& operator=(const PixelForceTorqueCalculator&) = delete;

    int calc_aero_torque_force(const glm::vec3& v_rel__m_per_s, float surface_temp__K, AeroConditions& aero,
                               glm::vec3& torque__Nm, glm::vec3& force__N) override;

    void set_flow_regime(FlowRegime regime) override { m_flow_regime.store(regime); }
    [[nodiscard]] FlowRegime get_flow_regime() const override { return m_flow_regime.load(); }

    /**
     * Samples per pixel actually used, after clamping to the driver limit and to the memory
     * the G-buffer could be allocated in.
     */
    [[nodiscard]] unsigned int num_msaa_samples() const { return m_num_msaa_samples; }

    /**
     * Pressure (normal component of the force per wetted area) [N/m^2] of the last
     * evaluation, num_pixel x num_pixel, row-major with row 0 at the bottom of the
     * flow view. Zero outside windward surfaces. A partly covered pixel shows the
     * pressure of its surface, not scaled by the coverage; with MSAA, a pixel where several
     * windward surfaces meet shows their sample-weighted mean. Empty unless
     * keep_pressure_image is set.
     */
    [[nodiscard]] const std::vector<float>& pressure_image() const { return m_pressure_image; }

    /** Wetted area of the windward surfaces the flow reaches, from the last evaluation [m^2]. */
    [[nodiscard]] double last_wetted_area() const { return m_wetted_area__m2; }

    /** Area normal to the flow covered by windward surfaces (coverage weighted), from the last evaluation [m^2]. */
    [[nodiscard]] double last_projected_area() const { return m_projected_area__m2; }

private:
    struct GpuState;

    IGeometryShadingData& m_geometry;
    IGSIModel& m_gsi_model;
    const unsigned int m_num_pixel;
    const bool m_keep_pressure_image;
    const float m_min_cos_delta;
    unsigned int m_num_msaa_samples = 1;   // after clamping; set in the constructor
    std::atomic<FlowRegime> m_flow_regime; // run-time switchable; atomic so a concurrent set is not a data race
    std::unique_ptr<GpuState> m_gpu;

    std::vector<float> m_pressure_image;
    double m_wetted_area__m2 = 0.0;
    double m_projected_area__m2 = 0.0;
};

} // namespace vat::loads
