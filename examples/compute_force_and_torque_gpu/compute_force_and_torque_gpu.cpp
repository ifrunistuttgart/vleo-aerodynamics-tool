#include <memory>
#include <filesystem>
#include <source_location>
#include <string>
#include <chrono>
#include <thread>
#define FMT_UNICODE 0 // avoid error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include "rotatable_mesh_satellite.h"
#include "gpu_aero_load_calculator.h"
#include "gsi.h"
#include "shading_pipeline.h"
#include "hybrid_aero_load_calculator.h"
#include "show_mesh.h"


// Resolves a filename relative to this source file's own location on disk.
std::filesystem::path get_path(const std::string& filename) {
	std::filesystem::path source_file(std::source_location::current().file_name());
	return source_file.parent_path() / filename;
}

int main() {
	//set spdlog to trace
	spdlog::set_level(spdlog::level::info);
	// 1. Load satellite geometry
	SPDLOG_INFO("Loading satellite model...");
	std::string obj_path = get_path("../geometry_files/tetraeder.obj").string();
	std::unique_ptr<RotatableMeshSatellite> satellite = std::make_unique<RotatableMeshSatellite>(obj_path);
	SPDLOG_INFO("Loaded {} triangles", satellite->get_num_triangles());

	// 3. Shading pipeline: determines which triangles face the incoming flow
	const int num_pixels = 800; // number of pixels: affects computation time and accuracy of shading

	//--------traditional torque and force calculation for comparison------------
	std::unique_ptr<gsi::cpu::Newton> gsi_model = std::make_unique<gsi::cpu::Newton>();

	std::unique_ptr<ShadingPipeline> pipeline =
	std::make_unique<ShadingPipeline>(*satellite, ShadingAlgorithmType::CoP, num_pixels);

	std::unique_ptr<HybridForceTorqueCalculator> hybrid_aero_calculator =
	std::make_unique<HybridForceTorqueCalculator>(*satellite, *pipeline, *gsi_model);

	//--------GPU torque and force calculation-------------------------------
	// 4. Aero load calculator: combines geometry, shading, and the GSI model
	std::unique_ptr<GPUAeroLoadCalculator> aero_calculator =
		std::make_unique<GPUAeroLoadCalculator>(*satellite, num_pixels);
	SPDLOG_INFO("Created GPU aero load calculator");

	// 5. Atmospheric/environment conditions
	std::unique_ptr<AeroConditions> aero_conditions = std::make_unique<AeroConditions>();
	aero_conditions->density__kg_per_m3 = 1.2482e-11f;
	aero_conditions->T_atmospheric__K = 934.0f;
	aero_conditions->particle_mass__kg = 16 * 1.6605390689252e-27f;
	const float surface_temp__K = 300.0f;
	SPDLOG_INFO("Created aero conditions");

	glm::vec3 velocity__m_per_s(7800.0f, 0.0f,0.0f);  // ~7.8 km/s orbital velocity

	//compute force and torque as reference
	glm::vec3 Hybrid_force__N(0.0f, 0.0f, 0.0f);
	glm::vec3 Hybrid_torque__Nm(0.0f, 0.0f, 0.0f);
	hybrid_aero_calculator->calc_aero_torque_force(velocity__m_per_s, surface_temp__K, *aero_conditions, Hybrid_torque__Nm, Hybrid_force__N);
	SPDLOG_INFO("Hybrid Force:  {}, {}, {} N", Hybrid_force__N.x, Hybrid_force__N.y, Hybrid_force__N.z);
	SPDLOG_INFO("Hybrid Torque: {}, {}, {} Nm", Hybrid_torque__Nm.x, Hybrid_torque__Nm.y, Hybrid_torque__Nm.z);

	//compute force and torque with GPU
	glm::vec3 force__N(0.0f, 0.0f, 0.0f);
	glm::vec3 torque__Nm(0.0f, 0.0f, 0.0f);
	aero_calculator->calc_aero_torque_force(velocity__m_per_s, surface_temp__K, *aero_conditions, torque__Nm, force__N);
	SPDLOG_INFO("Force:  {}, {}, {} N", force__N.x, force__N.y, force__N.z);
	SPDLOG_INFO("Torque: {}, {}, {} Nm", torque__Nm.x, torque__Nm.y, torque__Nm.z);
	//wait for 360 sec
	//std::this_thread::sleep_for(std::chrono::seconds(360));   // 2 s
}
