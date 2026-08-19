#include <memory>         // for std::unique_ptr
#include <filesystem>     // for std::filesystem::path
#include <source_location> // for std::source_location
#include <string>
#include <vector>

#include <glm/vec3.hpp>
#include <spdlog/spdlog.h>
#include "rotatable_mesh_satellite.h"
#include "show_mesh.h"

// Resolves a filename relative to this source file's own location on disk.
std::filesystem::path get_path(const std::string& filename) {
	std::filesystem::path source_file(std::source_location::current().file_name());
	return source_file.parent_path() / filename;
}

int main() {
	// 1. Load satellite model
	SPDLOG_INFO("Loading satellite model...");
	std::string obj_path = get_path("../geometry_files/shuttlecock_15k.obj").string();

	std::unique_ptr<RotatableMeshSatellite> satellite = std::make_unique<RotatableMeshSatellite>(obj_path);

	// 2. Dummy vector for visibility (all triangles visible)
	std::vector<float> dummy_visibility(satellite->get_num_triangles(), 1.0f);

	// 3. Visualize the satellite with dummy visibility
	SPDLOG_INFO("Visualizing satellite model...");
	glm::vec3 velocity__m_per_s(7800.0f, 0.0f, 0.0f);  // ~7.8 km/s orbital velocity
	ShowMeshWithShadingAndWind(*satellite, dummy_visibility, velocity__m_per_s);

	return 0;
}
