#include "pch.h"
#include "ShadingPipeline.h"
#include "ShadingAlgorithmFactory.h"
#include "StaticMeshSatellite.h"
#include <filesystem>
#include <iostream>
#include <memory>
#include <Sentman.h>
#include <Hybrid_force_torque_calculator.h>
#include "Core/Core.h"


// Get path relative to this source file
std::string GetTestDataPath(const std::string& filename) {
    std::filesystem::path source_file(__FILE__);
    std::filesystem::path data_file = source_file.parent_path() / filename;
    return data_file.string();
}

// Test class for StaticMeshSatellite
class HybridForceTorqueIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<StaticMeshSatellite> satellite;
	std::unique_ptr<Sentman> sentman;
	std::unique_ptr<HybridForceTorqueCalculator> force_torque_calculator;
	std::unique_ptr<ShadingPipeline> shading_pipeline;
	Eigen::Vector3f v_rel__m_per_s = Eigen::Vector3f(7800.0f, 0.0f, 0.0f);

    AeroConditions create_leo_conditions() {
        AeroConditions cond;
        cond.density__kg_per_m3 = 1.2482e-11f;
        cond.temperature__K = 934.0f;            // Exospheric temperature
        cond.particle_mass__kg = 16 * 1.6605390689252e-27f;       // Atomic oxygen mass
        cond.alpha_e = 0.9f;                      // Energy accommodation coefficient
        return cond;
    }

    void SetUp() override {
        std::string obj_path = GetTestDataPath("tetraeder.obj");
        std::cout << "[TEST] Loading OBJ from: " << obj_path << std::endl;
        satellite = std::make_unique<StaticMeshSatellite>(obj_path);
        std::cout << "[TEST] Loaded " << satellite->get_num_triangles() << " triangles" << std::endl;
		sentman = std::make_unique<Sentman>(1);
		shading_pipeline = std::make_unique<ShadingPipeline>(*satellite, ShadingAlgorithmType::Binary, 800);
		force_torque_calculator = std::make_unique<HybridForceTorqueCalculator>(*satellite, *shading_pipeline, *sentman);
    }
};


TEST_F(HybridForceTorqueIntegrationTest, TestShading) {
	Eigen::Vector3f torque(0.0f, 0.0f, 0.0f);
	Eigen::Vector3f force(0.0f, 0.0f, 0.0f);
	std::cout << "[TEST] Calculating aero torque and force..." << std::endl;
	AeroConditions conditions = create_leo_conditions();
	force_torque_calculator->calc_aero_torque_force(v_rel__m_per_s,300.0f, conditions, torque, force);
    //refernce values
    //aeroforce
    //    1.0e-03 *

    //    0.2212
    //    0
    //    - 0.0224

    //    aerotorque
    //    1.0e-04 *

    //    0
    //    0.3200
    //    0

	EXPECT_NEAR(force.x(), -0.2382e-3f, 1e-5f);
	EXPECT_NEAR(force.y(), 0.0f, 1e-5f);
	EXPECT_NEAR(force.z(), 0.0022e-3f, 1e-5f);
	EXPECT_NEAR(torque.x(), 0.0f, 1e-5f);
	EXPECT_NEAR(torque.y(), -0.3820e-4f, 1e-5f);
	EXPECT_NEAR(torque.z(), 0.0f, 1e-5f);
}