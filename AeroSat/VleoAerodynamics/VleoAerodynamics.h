#pragma once
#include "Core/ISatellite_shading_data.h"
#include "IGSI_Model.h"
#include "Core/IAero_calculator.h"
#include "IShadingPipeline.h"
#include <memory>
#include "Core/Core.h"
#include <string>
#include <glm/glm.hpp>

class VleoAerodynamics {
private:
	ISatelliteShadingData& m_satellite;
	std::unique_ptr<IShadingPipeline> m_shading_pipeline;
	std::unique_ptr<IGSIModel> m_gsi_model;
	std::unique_ptr<IAeroCalculator> m_aero_calculator;

public:
    VleoAerodynamics(ISatelliteShadingData& satellite, std::string gsi_model, int GPU_resolution);
    ~VleoAerodynamics() = default;

    int calculate_aero_torque_force(
        const glm::vec3& velocity__m_per_s,
        float surface_temperature__K,
        AeroConditions aero,
        glm::vec3& aero_torque__Nm,
        glm::vec3& aero_force__N
    );
	int visualize_shading(const glm::vec3& velocity__m_per_s);
};
