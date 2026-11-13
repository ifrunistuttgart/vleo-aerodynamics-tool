#define FMT_UNICODE 0 // avoid error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <limits>
#include <memory>
#include <glm/glm.hpp>

#include "sentman.h"
#include "hybrid_aero_load_calculator.h"
#include "core.h"
#include "shading_pipeline.h"
#include "shading_algorithm_factory.h"
#include "static_mesh_geometry.h"

using namespace vat;
using namespace vat::gsi_models;
using namespace vat::geometry;
using namespace vat::shading;
using namespace vat::loads;


// Get path relative to this source file
std::string GetTestDataPath(const std::string& filename) {
    std::filesystem::path source_file(__FILE__);
    std::filesystem::path data_file = source_file.parent_path() / filename;
    return data_file.string();
}

// Test class for StaticMeshGeometry
class HybridForceTorqueIntegrationTest : public ::testing::Test {
protected:
    std::unique_ptr<StaticMeshGeometry> geometry;
	std::unique_ptr<Sentman> sentman;
	std::unique_ptr<HybridForceTorqueCalculator> force_torque_calculator;
	std::unique_ptr<ShadingPipeline> shading_pipeline;
	glm::vec3 v_rel__m_per_s = glm::vec3(7800.0f, 0.0f, 0.0f);

    AeroConditions create_leo_conditions() {
        AeroConditions cond;
        cond.density__kg_per_m3 = 1.2482e-11f;
        cond.T_atmospheric__K = 934.0f;          // Exospheric temperature
        cond.particle_mass__kg = 16 * 1.6605390689252e-27f;       // Atomic oxygen mass
        return cond;
    }

    void SetUp() override {
        std::string obj_path = GetTestDataPath("../geometries/tetraeder.obj");
        //std::string obj_path = GetTestDataPath("International Space Station.obj");
		SPDLOG_INFO("[TEST] loading Test data from: {}", obj_path);
        geometry = std::make_unique<StaticMeshGeometry>(obj_path);
        SPDLOG_INFO("[TEST] Loaded {} triangles", geometry->get_num_triangles());
		sentman = std::make_unique<Sentman>(1, 0.9f);
		shading_pipeline = std::make_unique<ShadingPipeline>(*geometry, ShadingAlgorithmType::Binary, 800);
		force_torque_calculator = std::make_unique<HybridForceTorqueCalculator>(*geometry, *shading_pipeline, *sentman);
    }
};


TEST_F(HybridForceTorqueIntegrationTest, TestShading) {
	glm::vec3 torque(0.0f, 0.0f, 0.0f);
	glm::vec3 force(0.0f, 0.0f, 0.0f);
	SPDLOG_INFO("[TEST] Starting aero torque and force calculation test...");
	AeroConditions conditions = create_leo_conditions();
	force_torque_calculator->calc_aero_torque_force(v_rel__m_per_s,300.0f, conditions, torque, force);

	EXPECT_NEAR(force.x, -0.2382e-3f, 1e-5f);
	EXPECT_NEAR(force.y, 0.0f, 1e-5f);
	EXPECT_NEAR(force.z, 0.0022e-3f, 1e-5f);
	EXPECT_NEAR(torque.x, 0.0f, 1e-5f);
	EXPECT_NEAR(torque.y, -0.3820e-4f, 1e-5f);
	EXPECT_NEAR(torque.z, 0.0f, 1e-5f);
	SPDLOG_INFO("[TEST] Aero torque and force calculation test completed.");
}

TEST_F(HybridForceTorqueIntegrationTest, NonFiniteRelativeVelocityIsAnError) {
	AeroConditions conditions = create_leo_conditions();
	const float nan = std::numeric_limits<float>::quiet_NaN();
	const float inf = std::numeric_limits<float>::infinity();
	const glm::vec3 bad_velocities[] = {
		{nan, 0.0f, 0.0f}, {7800.0f, nan, 0.0f}, {inf, 0.0f, 0.0f}, {0.0f, -inf, 0.0f}, {nan, inf, 0.0f}};
	for (const glm::vec3& v_rel : bad_velocities) {
		glm::vec3 force(1.0f), torque(1.0f);
		EXPECT_NE(force_torque_calculator->calc_aero_torque_force(v_rel, 300.0f, conditions, torque, force), 0);
		EXPECT_TRUE(glm::all(glm::isnan(force)));
		EXPECT_TRUE(glm::all(glm::isnan(torque)));
	}

	// Zero speed is not an error: the load is zero.
	glm::vec3 force(1.0f), torque(1.0f);
	EXPECT_EQ(force_torque_calculator->calc_aero_torque_force(glm::vec3(0.0f), 300.0f, conditions, torque, force), 0);
	EXPECT_EQ(force, glm::vec3(0.0f));
	EXPECT_EQ(torque, glm::vec3(0.0f));
}

// Hyperthermal = hypothermal minus the unshadowed load of the leeward triangles.
TEST_F(HybridForceTorqueIntegrationTest, HyperthermalDropsLeewardTriangles) {
	AeroConditions conditions = create_leo_conditions();
	const glm::vec3 v_rel(7000.0f, 2000.0f, 3000.0f);

	EXPECT_EQ(force_torque_calculator->get_flow_regime(), FlowRegime::Hypothermal);
	glm::vec3 torque_hypo, force_hypo, torque_hyper, force_hyper;
	force_torque_calculator->calc_aero_torque_force(v_rel, 300.0f, conditions, torque_hypo, force_hypo);

	force_torque_calculator->set_flow_regime(FlowRegime::Hyperthermal);
	EXPECT_EQ(force_torque_calculator->get_flow_regime(), FlowRegime::Hyperthermal);
	force_torque_calculator->calc_aero_torque_force(v_rel, 300.0f, conditions, torque_hyper, force_hyper);

	glm::vec3 force_leeward(0.0f), torque_leeward(0.0f);
	auto areas = geometry->get_areas();
	auto normals = geometry->get_transformed_normals();
	auto centroids = geometry->get_transformed_centroids();
	int num_leeward = 0;
	for (unsigned int i = 0; i < geometry->get_num_triangles(); i++) {
		glm::vec3 n{normals[3 * i], normals[3 * i + 1], normals[3 * i + 2]};
		if (glm::dot(n, v_rel) > 0.0f) continue;
		num_leeward++;
		glm::vec3 c{centroids[3 * i], centroids[3 * i + 1], centroids[3 * i + 2]};
		glm::vec3 f, t;
		sentman->calc_aero_force_and_torque(areas[i], n, c, v_rel, 300.0f, conditions, f, t);
		force_leeward += f;
		torque_leeward += t;
	}
	ASSERT_GT(num_leeward, 0);

	const float scale = glm::length(force_hypo) + 1e-12f;
	EXPECT_LT(glm::length(force_hyper - (force_hypo - force_leeward)), 1e-4f * scale);
	EXPECT_LT(glm::length(torque_hyper - (torque_hypo - torque_leeward)), 1e-4f * scale);

	// Switching back restores the original result.
	force_torque_calculator->set_flow_regime(FlowRegime::Hypothermal);
	glm::vec3 torque_again, force_again;
	force_torque_calculator->calc_aero_torque_force(v_rel, 300.0f, conditions, torque_again, force_again);
	EXPECT_EQ(force_again, force_hypo);
}