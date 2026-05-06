#pragma once
#include "core/Iaero_calculator.h"
#include "core/Isatellite_shading_data.h"
#include "Ishading_pipeline.h"
#include "Igsi_model.h"
#include "core/core.h"

class HybridForceTorqueCalculator : public IAeroCalculator {
public:
    // Konstruktor nimmt jetzt einen Zeiger auf ISatellite
    HybridForceTorqueCalculator(ISatelliteShadingData& satellite, IShadingPipeline& shading_pipeline, IGSIModel& gsi_model);
    ~HybridForceTorqueCalculator() override;
    int calc_aero_torque_force(const glm::vec3& v_rel__m_per_s, float surface_temp__K, AeroConditions aero, glm::vec3& torque__Nm, glm::vec3& force__N) override;
    //int change_satellite(ISatellite* satellite) override;

private:
    IShadingPipeline& m_shading_pipeline;
    ISatelliteShadingData& m_satellite; // Zeiger statt Instanz
	IGSIModel& m_gsi_model;
};
