#include <memory>         // for std::unique_ptr
#include <filesystem>     // for std::filesystem::path
#include <source_location> // for std::source_location
#include <string>
#include <vector>

#include <glm/vec3.hpp>
#include <spdlog/spdlog.h>
#include "rotatable_mesh_geometry.h"
#include "show_mesh.h"

// Resolves a filename relative to this source file's own location on disk.
std::filesystem::path get_path(const std::string& filename) {
	std::filesystem::path source_file(std::source_location::current().file_name());
	return source_file.parent_path() / filename;
}

int main() {
	// 1. Load the geometry
	SPDLOG_INFO("Loading geometry...");
	std::string obj_path = get_path("../geometry_files/shuttlecock_15k.obj").string();

	std::unique_ptr<vat::RotatableMeshGeometry> geometry = std::make_unique<vat::RotatableMeshGeometry>(obj_path);

	// 2. Dummy vector for visibility (all triangles visible)
	std::vector<float> dummy_visibility(geometry->get_num_triangles(), 1.0f);

	// 3. Visualize the geometry with dummy visibility
	SPDLOG_INFO("Visualizing geometry...");
	glm::vec3 velocity__m_per_s(7800.0f, 0.0f, 0.0f);  // ~7.8 km/s orbital velocity
	vat::ShowMeshWithShadingAndWind(*geometry, dummy_visibility, velocity__m_per_s);

	return 0;
}
