#pragma once
#include "Iaero_load_calculator.h"
#include "Igeometry_shading_data.h"
#include "Ishading_pipeline.h"
#include "Igsi_model.h"
#include "core.h"

namespace vat::loads {

/**
 * @brief Load calculator that combines a per-triangle visibility from a shading pipeline with
 * a per-triangle GSI evaluation on the CPU.
 *
 * Each evaluation shades once, then loops over all triangles with the transformed normals and
 * centroids, evaluates the GSI model per triangle and multiplies its load by the triangle's
 * visibility. Leeward triangles (see is_leeward()) skip the visibility lookup and count with
 * visibility 1.0, or are skipped entirely in FlowRegime::Hyperthermal. Triangles whose
 * dot(n, v_rel) is NaN are skipped.
 *
 * The geometry, shading pipeline and GSI model are held by reference: the caller owns them
 * and must keep them alive for as long as the calculator is used.
 */
class HybridForceTorqueCalculator : public IAeroLoadCalculator {
public:
    /**
     * @brief Creates the calculator; does no GPU work itself.
     *
     * @param geometry Geometry to evaluate; referenced, must outlive the calculator.
     * @param shading_pipeline Visibility source; referenced, must outlive the calculator. It
     *        should be built for the same geometry.
     * @param gsi_model GSI model; referenced, must outlive the calculator.
     * @param flow_regime Hyperthermal skips leeward-facing triangles; see FlowRegime.
     */
    HybridForceTorqueCalculator(IGeometryShadingData& geometry, IShadingPipeline& shading_pipeline,
                                IGSIModel& gsi_model, FlowRegime flow_regime = FlowRegime::Hypothermal);
    ~HybridForceTorqueCalculator() override;
    int calc_aero_torque_force(const glm::vec3& v_rel__m_per_s, float surface_temp__K, AeroConditions& aero,
                               glm::vec3& torque__Nm, glm::vec3& force__N) override;

    void set_flow_regime(FlowRegime regime) override { m_flow_regime = regime; }
    [[nodiscard]] FlowRegime get_flow_regime() const override { return m_flow_regime; }

private:
    FlowRegime m_flow_regime;
    IShadingPipeline& m_shading_pipeline;
    IGeometryShadingData& m_geometry;
    IGSIModel& m_gsi_model;
};

} // namespace vat::loads
