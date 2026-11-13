// Projected area of a flat plate versus the angle between plate and flow, from the
// load calculators (hybrid with Binary without and with 8x MSAA, and CoP shading, and per pixel)
// and from the analytic solution.
//
// The ProjectedArea GSI model returns the force (projected area, 0, 0) for every
// element, so the x component of the total "force" is the area the flow sees. The
// plate (two_sided_plate.obj) is a unit square, so that area is |cos(theta)| m^2.
// The plate stays fixed; only the flow direction is rotated about the x axis.
//
//   pixi run run-example projected_area_plate
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numbers>
#include <source_location>
#include <string>
#include <vector>

#define FMT_UNICODE 0 // avoid error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include "gsi.h"
#include "geometry.h"
#include "shading_pipeline.h"
#include "hybrid_aero_load_calculator.h"
#include "pixel_force_torque_calculator.h"

// Resolves a filename relative to this source file's own location on disk.
std::filesystem::path get_path(const std::string& filename) {
	std::filesystem::path source_file(std::source_location::current().file_name());
	return source_file.parent_path() / filename;
}

// Writes the errors of the four calculators over the angle as one log-scale SVG plot.
// Errors below `floor` (exact zeros included) are drawn on the floor line.
void write_error_plot(const std::string& path, const std::vector<double>& theta__deg,
	const std::vector<std::vector<double>>& errors, const std::vector<std::string>& labels) {
	const double floor = 1e-10, ceiling = 1e-2;
	const double left = 80, right = 760, top = 30, bottom = 380;
	const char* colors[] = {"#1f77b4", "#9467bd", "#d62728", "#2ca02c"};
	const char* dashes[] = {"", "4 2", "8 4", "2 3"}; // the hybrid curves overlap, so tell them apart

	auto x_of = [&](double t) { return left + (right - left) * t / 180.0; };
	auto y_of = [&](double e) {
		const double clamped = std::clamp(e, floor, ceiling);
		return bottom - (bottom - top) * std::log10(clamped / floor) / std::log10(ceiling / floor);
	};

	std::ofstream svg(path);
	svg << "<svg xmlns='http://www.w3.org/2000/svg' width='900' height='450' font-family='sans-serif' font-size='12'>\n"
		<< "<rect width='900' height='450' fill='white'/>\n";
	for (int exponent = -10; exponent <= -2; ++exponent) { // horizontal grid, one line per decade
		const double y = y_of(std::pow(10.0, exponent));
		svg << "<line x1='" << left << "' x2='" << right << "' y1='" << y << "' y2='" << y << "' stroke='#ddd'/>"
			<< "<text x='" << left - 8 << "' y='" << y + 4 << "' text-anchor='end'>1e" << exponent << "</text>\n";
	}
	for (int t = 0; t <= 180; t += 30) {
		svg << "<line x1='" << x_of(t) << "' x2='" << x_of(t) << "' y1='" << top << "' y2='" << bottom << "' stroke='#eee'/>"
			<< "<text x='" << x_of(t) << "' y='" << bottom + 18 << "' text-anchor='middle'>" << t << "</text>\n";
	}
	svg << "<rect x='" << left << "' y='" << top << "' width='" << right - left << "' height='" << bottom - top
		<< "' fill='none' stroke='black'/>\n"
		<< "<text x='" << (left + right) / 2 << "' y='" << bottom + 40 << "' text-anchor='middle'>angle between flow and plate normal [deg]</text>\n"
		<< "<text transform='translate(22 " << (top + bottom) / 2 << ") rotate(-90)' text-anchor='middle'>absolute error of projected area [m^2]</text>\n"
		<< "<text x='" << (left + right) / 2 << "' y='18' text-anchor='middle' font-size='14'>Projected area of a unit plate: error vs analytic |cos(theta)|</text>\n";
	for (size_t k = 0; k < errors.size(); ++k) {
		svg << "<polyline fill='none' stroke='" << colors[k] << "' stroke-width='2' stroke-dasharray='" << dashes[k] << "' points='";
		for (size_t i = 0; i < theta__deg.size(); ++i) {
			svg << x_of(theta__deg[i]) << "," << y_of(errors[k][i]) << " ";
		}
		svg << "'/>\n";
		const double ly = top + 16 + 18 * k;
		svg << "<line x1='" << right - 170 << "' x2='" << right - 140 << "' y1='" << ly << "' y2='" << ly << "' stroke='" << colors[k]
			<< "' stroke-width='2' stroke-dasharray='" << dashes[k] << "'/>"
			<< "<text x='" << right - 134 << "' y='" << ly + 4 << "'>" << labels[k] << "</text>\n";
	}
	svg << "</svg>\n";
}

int main() {
	// 1. Geometry: unit plate in the XY plane, normals along +-z
	const std::string obj_path = get_path("../../matlab/examples/geometries/two_sided_plate.obj").string();
	vat::geometry::RotatableMeshGeometry geometry(obj_path);
	SPDLOG_INFO("Loaded {} triangles", geometry.get_num_triangles());

	// 2. The dummy model that turns the load into a projected area
	vat::gsi_models::ProjectedArea gsi_model;

	// 3. The four calculators (hybrid with Binary, Binary + 8x MSAA and CoP, and per pixel)
	const unsigned int num_pixel = 1000;
	vat::shading::ShadingPipeline binary_pipeline(geometry, vat::shading::ShadingAlgorithmType::Binary, num_pixel);
	vat::shading::ShadingPipeline msaa_pipeline(geometry, vat::shading::ShadingAlgorithmType::Binary, num_pixel, 8);
	vat::shading::ShadingPipeline cop_pipeline(geometry, vat::shading::ShadingAlgorithmType::CoP, num_pixel);
	vat::loads::HybridForceTorqueCalculator hybrid_binary(geometry, binary_pipeline, gsi_model);
	vat::loads::HybridForceTorqueCalculator hybrid_msaa(geometry, msaa_pipeline, gsi_model);
	vat::loads::HybridForceTorqueCalculator hybrid_cop(geometry, cop_pipeline, gsi_model);
	vat::loads::PixelForceTorqueCalculator pixel(geometry, gsi_model, num_pixel);

	// Neither the atmosphere nor the surface temperature enters this model.
	vat::AeroConditions aero_conditions{
		.density__kg_per_m3 = 1.2482e-11f,
		.T_atmospheric__K = 934.0f,
		.particle_mass__kg = 16 * 1.6605390689252e-27f,
	};
	const float surface_temp__K = 300.0f;
	const float speed__m_per_s = 7800.0f;
	const double plate_area__m2 = 1.0;

	// Projected area = x component of the total force
	auto projected_area = [&](vat::IAeroLoadCalculator& calculator, const glm::vec3& v_rel__m_per_s) {
		glm::vec3 force__N(0.0f);
		glm::vec3 torque__Nm(0.0f);
		calculator.calc_aero_torque_force(v_rel__m_per_s, surface_temp__K, aero_conditions, torque__Nm, force__N);
		return static_cast<double>(force__N.x);
	};

	// 4. Sweep the flow from face-on (0 deg) to the back face (180 deg)
	std::printf("%7s %10s | %10s %10s %10s %10s | %10s %10s %10s %10s\n", "theta", "analytic",
		"binary", "msaa", "cop", "pixel", "err_binary", "err_msaa", "err_cop", "err_pixel");
	double max_error_binary = 0.0, max_error_msaa = 0.0, max_error_cop = 0.0, max_error_pixel = 0.0;
	std::vector<double> angles__deg;
	std::vector<std::vector<double>> errors(4);
	for (float theta__deg = -1; theta__deg <= 181; theta__deg += 0.5) {
		const double theta__rad = theta__deg * std::numbers::pi / 180.0;
		const glm::vec3 v_rel__m_per_s(0.0f,
			speed__m_per_s * static_cast<float>(std::sin(theta__rad)),
			-speed__m_per_s * static_cast<float>(std::cos(theta__rad)));

		const double analytic = plate_area__m2 * std::abs(std::cos(theta__rad));
		const double binary = projected_area(hybrid_binary, v_rel__m_per_s);
		const double msaa = projected_area(hybrid_msaa, v_rel__m_per_s);
		const double cop = projected_area(hybrid_cop, v_rel__m_per_s);
		const double pixel_area = projected_area(pixel, v_rel__m_per_s);

		const double err_binary = std::abs(binary - analytic);
		const double err_msaa = std::abs(msaa - analytic);
		const double err_cop = std::abs(cop - analytic);
		const double err_pixel = std::abs(pixel_area - analytic);
		max_error_binary = std::max(max_error_binary, err_binary);
		max_error_msaa = std::max(max_error_msaa, err_msaa);
		max_error_cop = std::max(max_error_cop, err_cop);
		max_error_pixel = std::max(max_error_pixel, err_pixel);
		angles__deg.push_back(theta__deg);
		errors[0].push_back(err_binary);
		errors[1].push_back(err_msaa);
		errors[2].push_back(err_cop);
		errors[3].push_back(err_pixel);

		std::printf("%7.1f %10.5f | %10.5f %10.5f %10.5f %10.5f | %10.2e %10.2e %10.2e %10.2e\n", theta__deg, analytic,
			binary, msaa, cop, pixel_area, err_binary, err_msaa, err_cop, err_pixel);
	}

	std::printf("\nMax absolute error [m^2]: hybrid/binary %.3e, hybrid/msaa %.3e, hybrid/cop %.3e, pixel %.3e\n",
		max_error_binary, max_error_msaa, max_error_cop, max_error_pixel);

	// 5. Plot the errors (written to the working directory, i.e. the repo root under pixi)
	const std::string plot_path = "projected_area_plate_errors.svg";
	write_error_plot(plot_path, angles__deg, errors, {"hybrid + binary", "hybrid + binary 8x", "hybrid + CoP", "pixel"});
	SPDLOG_INFO("Wrote {}", std::filesystem::absolute(plot_path).string());
	return 0;
}
