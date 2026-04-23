#include <iostream>
#include <memory>
#include <chrono>
#include <iomanip>
#include <vector>
#include <stdexcept>
#include <span>
#include <glm/glm.hpp>
#define FMT_UNICODE 0 // avoid error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include "VleoAerodynamics.h"
#include "RotatableMeshSatellite.h"
#include "ShadingPipeline.h"
#include "ShadingAlgorithmFactory.h"
#include "Sentman.h"
#include "Hybrid_force_torque_calculator.h"
#include "showMesh.h"
#include "Core/Core.h"
#include <filesystem>


// Get path relative to this source file
std::string GetPath(const std::string& filename) {
    std::filesystem::path source_file(__FILE__);
    std::filesystem::path data_file = source_file.parent_path() / filename;
    return data_file.string();
}

int main() {
    spdlog::set_level(spdlog::level::debug); // Set *global* log level to debug

    // change log pattern
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%-5l%$] [%!] %v");

    // ============================================================================
    // STEP 1: Load Satellite Model
    // ============================================================================
    std::cout << "[1] Loading satellite model..." << std::endl;

    std::string satellite_path = GetPath("ISS_cut_no_materials.obj");
    std::unique_ptr<RotatableMeshSatellite> satellite =
        std::make_unique<RotatableMeshSatellite>(satellite_path);

    std::cout << "     Loaded " << satellite->get_num_triangles() << " triangles" << std::endl;


    // ============================================================================
    // STEP 2: Initialize Aerodynamic Calculator
    // ============================================================================
	VleoAerodynamics calculator(*satellite, "Sentman", 1000);

    // ============================================================================
    // STEP 3: Define Orbital Conditions (Low Earth Orbit)
    // ============================================================================
    AeroConditions leo_conditions;
    leo_conditions.density__kg_per_m3 = 1.2482e-11f;      // Typical at 400 km altitude
    leo_conditions.temperature__K = 934.0f;                // Exospheric temperature
    leo_conditions.particle_mass__kg = 16 * 1.6605390689252e-27f;  // Atomic oxygen
    leo_conditions.alpha_e = 0.9f;                          // Energy accommodation coefficient
    glm::vec3 velocity__m_per_s(0.0f, -7800.0f, 0.0f);  // ~7.8 km/s orbital velocity
    float surface_temperature__K = 300.0f;                     // 300 K (~27°C)

    // ============================================================================
    // STEP 4: rotate mesh
    // ============================================================================
	//rotate one solar panel by 180 degrees around its hinge axis (x-axis)
    satellite->turn_surface_around_axis(1, -3.14159265358979323846f / 3.0f, { 0.0f, -0.1789f, 1.5f }, { 1.0f, 0.0f, 0.0f });
    satellite->turn_surface_around_axis(2, -3.14159265358979323846f / 3.0f, { 0.0f, -0.1789f, 1.5f }, { 1.0f, 0.0f, 0.0f });

    // ============================================================================
    // STEP 5: Calculate Aerodynamic Forces & Torques
    // ============================================================================
    std::cout << "[5] CALCULATING AERODYNAMIC FORCES & TORQUES..." << std::endl;
    std::cout << std::string(50, '-') << std::endl;

    glm::vec3 force__N(0.0f);
    glm::vec3 torque__Nm(0.0f);

    auto calc_start = std::chrono::high_resolution_clock::now();
    int cycles = 10;
    int result = 0;
    for (int i = 0; i < cycles; ++i) {
        result = calculator.calculate_aero_torque_force(
            velocity__m_per_s,
            surface_temperature__K,
            leo_conditions,
            torque__Nm,
            force__N
        );
    }
    auto calc_end = std::chrono::high_resolution_clock::now();
    auto calc_duration = std::chrono::duration_cast<std::chrono::microseconds>(calc_end - calc_start);

    std::cout << std::string(50, '-') << std::endl;
    std::cout << std::endl;

    // ============================================================================
	// STEP 6: Display Shading Visualization
    // ============================================================================
	calculator.visualize_shading(velocity__m_per_s);

    // ============================================================================
    // STEP 7: Display Results
    // ============================================================================
    std::cout << "[RESULTS]  Calculation successful" << std::endl;
    std::cout << std::endl;

    std::cout << "AERODYNAMIC FORCE (N):" << std::endl;
    std::cout << "    Fx: " << std::scientific << std::setprecision(6) << force__N.x << std::endl;
    std::cout << "    Fy: " << std::scientific << std::setprecision(6) << force__N.y << std::endl;
    std::cout << "    Fz: " << std::scientific << std::setprecision(6) << force__N.z << std::endl;
    std::cout << "    |F|: " << std::scientific << std::setprecision(6) << glm::length(force__N) << std::endl;
    std::cout << std::endl;

    std::cout << "AERODYNAMIC TORQUE (N·m):" << std::endl;
    std::cout << "    Tx: " << std::scientific << std::setprecision(6) << torque__Nm.x << std::endl;
    std::cout << "    Ty: " << std::scientific << std::setprecision(6) << torque__Nm.y << std::endl;
    std::cout << "    Tz: " << std::scientific << std::setprecision(6) << torque__Nm.z << std::endl;
    std::cout << "    |T|: " << std::scientific << std::setprecision(6) << glm::length(torque__Nm) << std::endl;
    std::cout << std::endl;

    std::cout << std::string(50, '=') << std::endl;
    std::cout << " TIMING BREAKDOWN:" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    std::cout << "    Force/Torque calc:       " << std::setw(8) << std::fixed << std::setprecision(3)
        << calc_duration.count()/cycles / 1000.0f << " ms average per force calculation" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    return 0;
}