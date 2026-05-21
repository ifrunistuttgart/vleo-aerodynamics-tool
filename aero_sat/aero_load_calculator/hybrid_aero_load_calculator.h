#pragma once
#include "Iaero_load_calculator.h"
#include "Isatellite_shading_data.h"
#include "Ishading_pipeline.h"
#include "Igsi_model.h"
#include "core.h"

class HybridForceTorqueCalculator : public IAeroLoadCalculator {
public:
    /**
     * Constructor for the hybrid force and torque calculator.
     * @param satellite Reference to the satellite shading data.
     * @param shading_pipeline Reference to the shading pipeline.
     * @param gsi_model Reference to the GSI model.
     */
    HybridForceTorqueCalculator(ISatelliteShadingData& satellite, IShadingPipeline& shading_pipeline, IGSIModel& gsi_model);
    ~HybridForceTorqueCalculator() override;
    int calc_aero_torque_force(const glm::vec3& v_rel__m_per_s, float surface_temp__K, AeroConditions aero, glm::vec3& torque__Nm, glm::vec3& force__N) override;

private:
    IShadingPipeline& m_shading_pipeline;
    ISatelliteShadingData& m_satellite;
    IGSIModel& m_gsi_model;
};
