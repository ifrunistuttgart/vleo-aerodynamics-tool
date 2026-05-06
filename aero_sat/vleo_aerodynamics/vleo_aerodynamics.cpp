#define FMT_UNICODE 0 // avoid error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include "vleo_aerodynamics.h"
#include "sentman.h"
#include "show_mesh.h"
#include "hybrid_force_torque_calculator.h"
#include "shading_pipeline.h"


VleoAerodynamics::VleoAerodynamics(ISatelliteShadingData& satellite, std::string gsi_model, int GPU_resolution)
	: m_satellite(satellite)
{
	if (gsi_model == "Sentman") {
		m_gsi_model = std::make_unique<Sentman>(1);
	} else {
		SPDLOG_ERROR("Unsupported GSI model: {}", gsi_model);
		throw std::invalid_argument("Unsupported GSI model: " + gsi_model);
	}
	m_shading_pipeline = std::make_unique<ShadingPipeline>(m_satellite, ShadingAlgorithmType::Binary, GPU_resolution);

	m_aero_calculator = std::make_unique<HybridForceTorqueCalculator>(m_satellite, *m_shading_pipeline, *m_gsi_model);
}

int VleoAerodynamics::calculate_aero_torque_force(const glm::vec3& velocity__m_per_s, float surface_temperature__K, AeroConditions aero, glm::vec3& aero_torque__Nm, glm::vec3& aero_force__N)
{
	return  m_aero_calculator->calc_aero_torque_force(
		velocity__m_per_s,
		surface_temperature__K,
		aero,
		aero_torque__Nm,
		aero_force__N);
}

int VleoAerodynamics::visualize_shading(const glm::vec3& velocity__m_per_s)
{
	std::vector<float> triangle_visibility(m_satellite.get_num_triangles(), 0.0f);
	int shade_result = m_shading_pipeline->shade(std::span<float>(triangle_visibility), glm::normalize(velocity__m_per_s));
	if (shade_result == 0) {
		ShowMeshWithShadingAndWind(m_satellite, triangle_visibility, velocity__m_per_s);
	}
	else {
		SPDLOG_ERROR("Shading failed with code: {}", shade_result);
		return shade_result;
	}
	return 0;
}
