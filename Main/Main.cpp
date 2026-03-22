#include <iostream>
#include <memory>
#include <chrono>
#include <iomanip>
#include <eigen3/Eigen/Dense>
#include "StaticMeshSatellite.h"
#include "ShadingPipeline.h"
#include "ShadingAlgorithmFactory.h"
#include "Sentman.h"
#include "Hybrid_force_torque_calculator.h"
#include "Core/Core.h"
#include <filesystem>

// Get path relative to this source file
std::string GetPath(const std::string& filename) {
    std::filesystem::path source_file(__FILE__);
    std::filesystem::path data_file = source_file.parent_path() / filename;
    return data_file.string();
}
int main() {
    try {
        std::cout << "=== AeroSat Aerodynamic Force & Torque Calculator ===" << std::endl;
        std::cout << std::endl;

        // ============================================================================
        // STEP 1: Load Satellite Model
        // ============================================================================
        std::cout << "[1] Loading satellite model..." << std::endl;

        std::string satellite_path = GetPath("two_spheres_1.obj");  // TODO: Update with actual path
        std::unique_ptr<StaticMeshSatellite> satellite =
            std::make_unique<StaticMeshSatellite>(satellite_path);

        std::cout << "    ✓ Loaded " << satellite->get_num_triangles() << " triangles" << std::endl;


        // ============================================================================
        // STEP 2: Initialize Shading Pipeline (for shadow calculations)
        // ============================================================================
        std::cout << "[2] Initializing shading pipeline..." << std::endl;
        std::unique_ptr<ShadingPipeline> shading_pipeline =
            std::make_unique<ShadingPipeline>(*satellite, ShadingAlgorithmType::Binary, 800);

        std::cout << "    ✓ Shading pipeline ready" << std::endl;

        // ============================================================================
        // STEP 3: Initialize Gas-Surface Interaction (GSI) Model
        // ============================================================================
        std::cout << "[3] Initializing GSI model (Sentman)..." << std::endl;
        std::unique_ptr<Sentman> gsi_model = std::make_unique<Sentman>(1);
        std::cout << "    ✓ Sentman GSI model initialized" << std::endl;
        std::cout << std::endl;

        // ============================================================================
        // STEP 4: Create Force & Torque Calculator
        // ============================================================================
        std::cout << "[4] Creating aerodynamic force/torque calculator..." << std::endl;
        std::unique_ptr<HybridForceTorqueCalculator> calculator =
            std::make_unique<HybridForceTorqueCalculator>(*satellite, *shading_pipeline, *gsi_model);
        std::cout << "    ✓ Calculator initialized" << std::endl;
        std::cout << std::endl;

        // ============================================================================
        // STEP 5: Define Orbital Conditions (Low Earth Orbit)
        // ============================================================================
        AeroConditions leo_conditions;
        leo_conditions.density__kg_per_m3 = 1.2482e-11f;      // Typical at 400 km altitude
        leo_conditions.temperature__K = 934.0f;                // Exospheric temperature
        leo_conditions.particle_mass__kg = 16 * 1.6605390689252e-27f;  // Atomic oxygen
        leo_conditions.alpha_e = 0.9f;                          // Energy accommodation coefficient

        // ============================================================================
        // STEP 6: Define Satellite State
        // ============================================================================
        Eigen::Vector3f velocity__m_per_s(7800.0f, 0.0f, 0.0f);  // ~7.8 km/s orbital velocity
        float surface_temperature__K = 300.0f;                     // 300 K (~27°C)


        // ============================================================================
        // STEP 7: Calculate Aerodynamic Forces & Torques
        // ============================================================================
        std::cout << "[7] CALCULATING AERODYNAMIC FORCES & TORQUES..." << std::endl;
        std::cout << std::string(50, '-') << std::endl;

        Eigen::Vector3f force__N = Eigen::Vector3f::Zero();
        Eigen::Vector3f torque__Nm = Eigen::Vector3f::Zero();

        auto calc_start = std::chrono::high_resolution_clock::now();

        int result = calculator->calc_aero_torque_force(
            velocity__m_per_s,
            surface_temperature__K,
            leo_conditions,
            torque__Nm,
            force__N
        );

        auto calc_end = std::chrono::high_resolution_clock::now();
        auto calc_duration = std::chrono::duration_cast<std::chrono::microseconds>(calc_end - calc_start);

        std::cout << std::string(50, '-') << std::endl;
        std::cout << std::endl;

        // ============================================================================
        // STEP 8: Display Results
        // ============================================================================
        if (result == 0) {
            std::cout << "[RESULTS] ✓ Calculation successful" << std::endl;
            std::cout << std::endl;

            std::cout << "AERODYNAMIC FORCE (N):" << std::endl;
            std::cout << "    Fx: " << std::scientific << std::setprecision(6) << force__N.x() << std::endl;
            std::cout << "    Fy: " << std::scientific << std::setprecision(6) << force__N.y() << std::endl;
            std::cout << "    Fz: " << std::scientific << std::setprecision(6) << force__N.z() << std::endl;
            std::cout << "    |F|: " << std::scientific << std::setprecision(6) << force__N.norm() << std::endl;
            std::cout << std::endl;

            std::cout << "AERODYNAMIC TORQUE (N·m):" << std::endl;
            std::cout << "    Tx: " << std::scientific << std::setprecision(6) << torque__Nm.x() << std::endl;
            std::cout << "    Ty: " << std::scientific << std::setprecision(6) << torque__Nm.y() << std::endl;
            std::cout << "    Tz: " << std::scientific << std::setprecision(6) << torque__Nm.z() << std::endl;
            std::cout << "    |T|: " << std::scientific << std::setprecision(6) << torque__Nm.norm() << std::endl;
            std::cout << std::endl;

            std::cout << std::string(50, '=') << std::endl;
            std::cout << "⏱ TIMING BREAKDOWN:" << std::endl;
            std::cout << std::string(50, '=') << std::endl;
            std::cout << "    Force/Torque calc:       " << std::setw(8) << std::fixed << std::setprecision(3)
                << calc_duration.count() / 1000.0f << " ms" << std::endl;
            std::cout << std::string(50, '=') << std::endl;
        }
        else {
            std::cout << "[ERROR] Calculation failed with code: " << result << std::endl;
            return 1;
        }

        std::cout << std::endl << "Program completed successfully!" << std::endl;
        return 0;

    }
    catch (const std::exception& e) {
        std::cerr << "[FATAL ERROR] " << e.what() << std::endl;
        return -1;
    }
}