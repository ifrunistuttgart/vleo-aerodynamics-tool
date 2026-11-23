#pragma once
#include "Iaero_load_calculator.h"
#include "Igeometry_shading_data.h"
#include "Ishading_pipeline.h"
#include "Igsi_model.h"
#include "core.h"

namespace vat::loads {

class HybridForceTorqueCalculator : public IAeroLoadCalculator {
public:
    /**
     * Constructor for the hybrid force and torque calculator.
     * @param geometry Reference to the geometry shading data.
     * @param shading_pipeline Reference to the shading pipeline.
     * @param gsi_model Reference to the GSI model.
     * @param flow_regime Hyperthermal skips leeward-facing triangles; see FlowRegime.
     */
    HybridForceTorqueCalculator(IGeometryShadingData& geometry, IShadingPipeline& shading_pipeline, IGSIModel& gsi_model,
        FlowRegime flow_regime = FlowRegime::Hypothermal);
    ~HybridForceTorqueCalculator() override;
    int calc_aero_torque_force(const glm::vec3& v_rel__m_per_s, float surface_temp__K, AeroConditions& aero, glm::vec3& torque__Nm, glm::vec3& force__N) override;

    void set_flow_regime(FlowRegime regime) override { m_flow_regime = regime; }
    [[nodiscard]] FlowRegime get_flow_regime() const override { return m_flow_regime; }

private:
    FlowRegime m_flow_regime;
    IShadingPipeline& m_shading_pipeline;
    IGeometryShadingData& m_geometry;
    IGSIModel& m_gsi_model;
};

} // namespace vat::loads
