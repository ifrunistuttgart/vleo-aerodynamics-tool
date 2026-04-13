#include "Hybrid_force_torque_calculator.h"
#include <vector>
#include <span>
#include <iostream>
#include <glm/glm.hpp>

/**
 * @param satellite
 */
HybridForceTorqueCalculator::HybridForceTorqueCalculator(
	ISatelliteShadingData& satellite,
	IShadingPipeline& shading_pipeline,
	IGSIModel& gsi_model)
	: m_shading_pipeline(shading_pipeline),
	m_satellite(satellite),
	m_gsi_model(gsi_model)
{

}

HybridForceTorqueCalculator::~HybridForceTorqueCalculator() {

}

int HybridForceTorqueCalculator::calc_aero_torque_force(const glm::vec3& v_rel__m_per_s, float surface_temp__K, AeroConditions aero, glm::vec3& torque__Nm, glm::vec3& force__N) {
	std::vector<float> triangle_visibility(m_satellite.get_num_triangles(), 0.0f);
	torque__Nm = glm::vec3(0.0f);
	force__N = glm::vec3(0.0f);

	m_shading_pipeline.shade(std::span<float>(triangle_visibility), glm::normalize(v_rel__m_per_s));
	std::span<const float> areas = m_satellite.get_areas();
	std::span<const float> normals = m_satellite.get_normals();
	std::span<const float> centroids = m_satellite.get_centroids();

	//loop over triangles
	for (unsigned int i = 0; i < m_satellite.get_num_triangles(); i++){
		glm::vec3 normal{normals[3*i], normals[3*i+1], normals[3*i+2]};
		glm::vec3 centroid{ centroids[3 * i], centroids[3 * i + 1], centroids[3 * i + 2] };
		float area = areas[i];

		glm::vec3 aero_force__N;
		glm::vec3 aero_torque__Nm;

		float visibility;
		if (glm::dot(normal, v_rel__m_per_s) <= 0) {
			//backward facing
			visibility = 1.0f;
		}
		else
		{
			visibility = triangle_visibility[i];
		}
		m_gsi_model.calc_aero_force_and_torque(area, normal, centroid, v_rel__m_per_s, surface_temp__K, aero, aero_force__N, aero_torque__Nm);

		for (int j = 0; j < 3; j++) {
			force__N[j] += visibility * aero_force__N[j];
			torque__Nm[j] += visibility * aero_torque__Nm[j];
		}

	}
	return 0;
}

