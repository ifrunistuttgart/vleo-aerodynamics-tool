#include <memory>
#include <filesystem>
#include <source_location>
#include <string>
#include <vector>

#define FMT_UNICODE 0 // avoid error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <chrono>
#include "sentman.h"
#include "rotatable_mesh_satellite.h"
#include "shading_pipeline.h"
#include "hybrid_aero_load_calculator.h"
#include "gpu_aero_load_calculator.h"


// Resolves a filename relative to this source file's own location on disk.
std::filesystem::path get_path(const std::string& filename) {
	std::filesystem::path source_file(std::source_location::current().file_name());
	return source_file.parent_path() / filename;
}

int main() {
	
	// 1. Load satellite geometry
	SPDLOG_INFO("Loading satellite model...");
	std::string obj_path = get_path("../geometry_files/iss_26k.obj").string();
	std::unique_ptr<RotatableMeshSatellite> satellite = std::make_unique<RotatableMeshSatellite>(obj_path);
	SPDLOG_INFO("Loaded {} triangles", satellite->get_num_triangles());

	// 2. Gas-surface interaction model
	std::unique_ptr<Sentman> gsi_model = std::make_unique<Sentman>(1,0.9f);
	SPDLOG_INFO("Initialized Sentman GSI model");

	// 3. Shading pipeline: determines which triangles face the incoming flow
	const int num_pixels = 800; // number of pixels: affects computation time and accuracy of shading
	std::unique_ptr<ShadingPipeline> pipeline = std::make_unique<ShadingPipeline>(*satellite, ShadingAlgorithmType::CoP, num_pixels);
	SPDLOG_INFO("Created shading pipeline (algorithm=CoP, pixels={})", num_pixels);

	// 4. Aero load calculator: combines geometry, shading, and the GSI model
	std::unique_ptr<HybridForceTorqueCalculator> aero_calculator = std::make_unique<HybridForceTorqueCalculator>(*satellite, *pipeline, *gsi_model);
	SPDLOG_INFO("Created hybrid aero load calculator");

	// 5. create GPU based
	std::unique_ptr<GPUAeroLoadCalculator> gpu_aero_calculator = std::make_unique<GPUAeroLoadCalculator>(*satellite, num_pixels);
	SPDLOG_INFO("Created GPU aero load calculator");

	// 5. Atmospheric/environment conditions
	std::unique_ptr<AeroConditions> aero_conditions = std::make_unique<AeroConditions>();
	aero_conditions->density__kg_per_m3 = 1.2482e-11f;
	aero_conditions->T_atmospheric__K = 934.0f;
	aero_conditions->particle_mass__kg = 16 * 1.6605390689252e-27f;
	const float surface_temp__K = 300.0f;
	SPDLOG_INFO("Created aero conditions");

	glm::vec3 velocity__m_per_s(0.0f, -7800.0f, 0.0f);  // ~7.8 km/s orbital velocity

	// 6. Shade the mesh and compute the resulting force/torque
	std::vector<float> triangle_visibility;
	constexpr int benchmark_iterations = 1000;
	auto shading_start = std::chrono::high_resolution_clock::now();
	spdlog::set_level(spdlog::level::off);

	for (int i = 0; i < benchmark_iterations; i++) {
		triangle_visibility = pipeline->shade(glm::normalize(velocity__m_per_s));
	}
	spdlog::set_level(spdlog::level::info);

	auto shading_end = std::chrono::high_resolution_clock::now();
	auto shading_duration = std::chrono::duration<double, std::milli>(shading_end - shading_start);
	SPDLOG_INFO("Average shading duration in {} milliseconds", shading_duration.count() / benchmark_iterations);

	glm::vec3 force__N(0.0f, 0.0f, 0.0f);
	glm::vec3 torque__Nm(0.0f, 0.0f, 0.0f);

	auto start = std::chrono::high_resolution_clock::now();
	spdlog::set_level(spdlog::level::off);
	for (int i = 0; i < benchmark_iterations; i++) {
		aero_calculator->calc_aero_torque_force(velocity__m_per_s, surface_temp__K, *aero_conditions, torque__Nm, force__N);
	}
	spdlog::set_level(spdlog::level::info);

	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration<double, std::milli>(end - start);
	SPDLOG_INFO("Average aero load calculation duration in {} milliseconds", duration.count() / benchmark_iterations);

	auto gpu_start = std::chrono::high_resolution_clock::now();

	spdlog::set_level(spdlog::level::off);
	for (int i = 0; i < benchmark_iterations; i++) {
		gpu_aero_calculator->calc_aero_torque_force(velocity__m_per_s, surface_temp__K, *aero_conditions, torque__Nm, force__N);
	}
	spdlog::set_level(spdlog::level::info);

	auto gpu_end = std::chrono::high_resolution_clock::now();
	auto gpu_duration = std::chrono::duration<double, std::milli>(gpu_end - gpu_start);
	SPDLOG_INFO("Average gpu aero load calculation duration in {} milliseconds", gpu_duration.count() / benchmark_iterations);

	return 0;
}
