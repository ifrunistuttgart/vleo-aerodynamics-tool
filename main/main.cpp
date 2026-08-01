#include <memory>
#include <filesystem>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include "sentman.h"
#include "rotatable_mesh_satellite.h"
#include "shading_pipeline.h"
#include "hybrid_aero_load_calculator.h"
#include "show_mesh.h"

// Get path relative to this source file
std::string get_path(const std::string& filename) {
	std::filesystem::path source_file(__FILE__);
	std::filesystem::path data_file = source_file.parent_path() / filename;
	return data_file.string();
}

int main() {
	SPDLOG_INFO("Loading satellite model...");

	std::string satellite_path = get_path("International Space Station.obj");
	std::unique_ptr<RotatableMeshSatellite> satellite =
		std::make_unique<RotatableMeshSatellite>(satellite_path);
	SPDLOG_INFO("Loaded {} triangles", satellite->get_num_triangles());

	std::unique_ptr<Sentman> gsi_model = std::make_unique<Sentman>(1);
	SPDLOG_INFO("Initialized Sentman model");

	const int num_pixel = 2000;
	std::unique_ptr<ShadingPipeline> pipeline = std::make_unique<ShadingPipeline>(*satellite, ShadingAlgorithmType::CoP,  num_pixel);
	SPDLOG_INFO("Created shading pipeline (algorithm=Binary, pixels={})", num_pixel);

	std::unique_ptr<HybridForceTorqueCalculator> aero_calculator = std::make_unique<HybridForceTorqueCalculator>(*satellite, *pipeline, *gsi_model);
	SPDLOG_INFO("Created hybrid aero load calculator");

	std::unique_ptr<AeroConditions> aero_conditions = std::make_unique<AeroConditions>();
	aero_conditions->density__kg_per_m3 = 1.2482e-11f;
	aero_conditions->temperature__K = 934.0f;
	aero_conditions->particle_mass__kg = 16 * 1.6605390689252e-27f;
	aero_conditions->alpha_e = 0.9f;
	const float surface_temp__K = 300.0f;
	SPDLOG_INFO("Created aero load calculator");

	glm::vec3 velocity__m_per_s(0.0f, -7800.0f, 0.0f);  // ~7.8 km/s orbital velocity

	std::vector<float> triangle_visibility = pipeline->shade(glm::normalize(velocity__m_per_s));
	SPDLOG_INFO("test shading completed");

	glm::vec3 force__N(0.0f, 0.0f, 0.0f);
	glm::vec3 torque__Nm(0.0f, 0.0f, 0.0f);
	aero_calculator->calc_aero_torque_force(velocity__m_per_s, surface_temp__K, *aero_conditions, torque__Nm, force__N);
	SPDLOG_INFO("calculate force: {}, {}, {} N", force__N.x, force__N.y, force__N.z);
	SPDLOG_INFO("calculate torque: {}, {}, {} Nm", torque__Nm.x, torque__Nm.y, torque__Nm.z);
	SPDLOG_INFO("test shading completed");
	ShowMeshWithShadingAndWind(*satellite, triangle_visibility, velocity__m_per_s);
	return 0;
}
