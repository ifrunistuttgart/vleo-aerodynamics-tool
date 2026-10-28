// Computes the aerodynamic force and torque on the shuttlecock with the per-pixel
// calculator, which integrates the GSI model over every pixel the flow reaches.
// Compared with compute_force_and_torque, it needs no shading pipeline: the
// calculator renders the geometry itself.
//
//   pixi run run-example compute_force_and_torque_pixel
#include <algorithm>
#include <filesystem>
#include <memory>
#include <source_location>
#include <string>

#define FMT_UNICODE 0 // avoid error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include "gsi.h"
#include "geometry.h"
#include "pixel_force_torque_calculator.h"

// Resolves a filename relative to this source file's own location on disk.
std::filesystem::path get_path(const std::string& filename) {
	std::filesystem::path source_file(std::source_location::current().file_name());
	return source_file.parent_path() / filename;
}

int main() {
	// 1. Load the geometry
	std::string obj_path = get_path("../geometry_files/shuttlecock_15k.obj").string();
	auto geometry = std::make_unique<vat::geometry::RotatableMeshGeometry>(obj_path);
	SPDLOG_INFO("Loaded {} triangles", geometry->get_num_triangles());

	// 2. Gas-surface interaction model: temperature ratio method 1, alpha_e = 0.9
	auto gsi_model = std::make_unique<vat::gsi_models::Sentman>(1, 0.9f);

	// 3. Per-pixel calculator. num_pixel is the accuracy/runtime knob: the geometry
	//    is rendered into num_pixel x num_pixel pixels along the flow.
	//    keep_pressure_image also keeps the pressure of every pixel.
	const unsigned int num_pixel = 2000;
	vat::loads::PixelLoadOptions options;
	options.keep_pressure_image = true;
	auto calculator = std::make_unique<vat::loads::PixelForceTorqueCalculator>(*geometry, *gsi_model, num_pixel, options);

	// 4. Atmosphere at roughly 300 km altitude, atomic oxygen
	vat::AeroConditions aero_conditions{
		.density__kg_per_m3 = 1.2482e-11f,
		.T_atmospheric__K = 934.0f,
		.particle_mass__kg = 16 * 1.6605390689252e-27f,
	};
	const float surface_temp__K = 300.0f;

	// 5. Force and torque. v_rel is the velocity of the satellite relative to the
	//    atmosphere, in the body frame; the torque is about the body origin.
	glm::vec3 velocity__m_per_s(0.0f, -7800.0f, 0.0f);
	glm::vec3 force__N(0.0f);
	glm::vec3 torque__Nm(0.0f);
	calculator->calc_aero_torque_force(velocity__m_per_s, surface_temp__K, aero_conditions, torque__Nm, force__N);

	SPDLOG_INFO("Force:  {}, {}, {} N", force__N.x, force__N.y, force__N.z);
	SPDLOG_INFO("Torque: {}, {}, {} Nm", torque__Nm.x, torque__Nm.y, torque__Nm.z);

	// 6. What the evaluation integrated over
	SPDLOG_INFO("Wetted area:  {} m^2 (surfaces the flow reaches)", calculator->last_wetted_area());
	SPDLOG_INFO("Frontal area: {} m^2 (the same, seen along the flow)", calculator->last_projected_area());

	// pressure_image() holds num_pixel x num_pixel values, row 0 at the bottom of
	// the view from upstream, zero where no surface faces the flow.
	float max_pressure = 0.0f;
	for (float p : calculator->pressure_image()) {
		max_pressure = std::max(max_pressure, p);
	}
	SPDLOG_INFO("Largest pressure: {} N/m^2", max_pressure);
	return 0;
}
