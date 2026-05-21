#include <memory>
#include <filesystem>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include "sentman.h"
#include "rotatable_mesh_satellite.h"
#include "shading_pipeline.h"
//#include "show_mesh.h"

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

	std::unique_ptr<ShadingPipeline> pipeline = std::make_unique<ShadingPipeline>(*satellite, ShadingAlgorithmType::Binary, 800);
	SPDLOG_INFO("Created shading pipeline (algorithm=Binary, pixels={})", 800);

	std::vector<float> triangle_visibility(satellite->get_num_triangles(), 0.0f);
	glm::vec3 velocity__m_per_s(0.0f, -7800.0f, 0.0f);  // ~7.8 km/s orbital velocity

	int shade_result = pipeline->shade(std::span<float>(triangle_visibility), glm::normalize(velocity__m_per_s));
	if (shade_result != 0) {
		SPDLOG_ERROR("Shading failed (code={})", shade_result);
	}

	//ShowMeshWithShadingAndWind(*satellite, triangle_visibility, velocity__m_per_s);
	return 0;
}
