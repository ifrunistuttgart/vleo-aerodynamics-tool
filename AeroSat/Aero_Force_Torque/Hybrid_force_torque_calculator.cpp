#include "Hybrid_force_torque_calculator.h"
#include <vector>
#include <span>
#include <iostream>
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

int HybridForceTorqueCalculator::calc_aero_torque_force(const Eigen::Vector3f& v_rel__m_per_s, float surface_temp__K, AeroConditions aero, Eigen::Vector3f& torque__Nm, Eigen::Vector3f& force__N) {
	std::vector<float> triangle_visibility(m_satellite.get_num_triangles(), 0.0f);
	torque__Nm.setZero();
	force__N.setZero();

	m_shading_pipeline.shade(std::span<float>(triangle_visibility), v_rel__m_per_s.normalized());
	std::span<const float> areas = m_satellite.get_areas();
	std::span<const float> normals = m_satellite.get_normals();
	std::span<const float> centroids = m_satellite.get_centroids();

	//loop over triangles
	for (unsigned int i = 0; i < m_satellite.get_num_triangles(); i++){
		Eigen::Vector3f normal{normals[3*i], normals[3*i+1], normals[3*i+2]};
		Eigen::Vector3f centroid{ centroids[3 * i], centroids[3 * i + 1], centroids[3 * i + 2] };
		float area = areas[i];

		Eigen::Vector3f aero_force__N;
		Eigen::Vector3f aero_torque__Nm;

		float visibility;
		if (normal.dot(v_rel__m_per_s) <= 0) {
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

