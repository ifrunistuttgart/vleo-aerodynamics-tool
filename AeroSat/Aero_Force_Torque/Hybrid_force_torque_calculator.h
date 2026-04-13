#pragma once
#include "Core/IAero_calculator.h"
#include "Core/ISatellite_shading_data.h"
#include "IShadingPipeline.h"
#include "IGSI_Model.h"
#include "Core/Core.h"

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
