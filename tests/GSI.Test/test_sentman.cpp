#include "pch.h"
#include "Sentman.h"
#include "Core/Core.h"


// Test Fixture
class SentmanTest : public ::testing::Test {
protected:
    // Helper: Create standard LEO conditions
    AeroConditions create_leo_conditions() {
        AeroConditions cond;
        cond.density__kg_per_m3 = 1.2482e-11f;
        cond.temperature__K = 934.0f;            // Exospheric temperature
        cond.particle_mass__kg = 16 * 1.6605390689252e-27f;       // Atomic oxygen mass
        cond.alpha_e = 0.9f;                      // Energy accommodation coefficient
        return cond;
    }
};

// Constructor Tests
TEST_F(SentmanTest, Constructor_ValidMethods_Construct) {
    EXPECT_NO_THROW(Sentman{ 1 });
    EXPECT_NO_THROW(Sentman{ 2 });
    EXPECT_NO_THROW(Sentman{ 3 });
}

// Torque Calculation Tests
TEST_F(SentmanTest, CalcForceAndTorque_TemperatureRatioMethod1) {
    // Arrange
    Sentman sentman{ 1 };
    Eigen::Vector3f normal(1.0f, 1.0f, 0.0f);
    normal = normal.normalized();
    Eigen::Vector3f centroid(1.0f, 1.0f, 1.0f);
    Eigen::Vector3f velocity(7800.0f, 0.0f, 0.0f);
    auto conditions = create_leo_conditions();

    Eigen::Vector3f force;
    Eigen::Vector3f torque;
	Eigen::Vector3f expected_torque = Eigen::Vector3f(0.0843f, 0.4526f, -0.5370f) * 1e-3f;
	Eigen::Vector3f expected_force = Eigen::Vector3f(0.4526f, -0.0843f, 0.0f)* 1e-3f;

    // Act
    sentman.calc_aero_force_and_torque(
        1.0f, normal, centroid, velocity, 300.0f, conditions, force, torque
    );

    // Assert
	EXPECT_TRUE(force.isApprox(expected_force, 1e-3f)) << "  Actual force: [" << force.x() << ", " << force.y() << ", " << force.z() << "]\n";
	EXPECT_TRUE(torque.isApprox(expected_torque, 1e-3f)) << "  Actual torque: [" << torque.x() << ", " << torque.y() << ", " << torque.z() << "]\n";
}

TEST_F(SentmanTest, CalcForceAndTorque_TemperatureRatioMethod11) {
    // Arrange
    Sentman sentman{ 1 };
    Eigen::Vector3f normal(0.0f, 0.0f, -1.0f);
    normal = normal.normalized();
    Eigen::Vector3f centroid(-0.166667f, 0.0f, 0.0f);
    Eigen::Vector3f velocity(7800.0f, 0.0f, 0.0f);
    auto conditions = create_leo_conditions();

    Eigen::Vector3f force;
    Eigen::Vector3f torque;
    Eigen::Vector3f expected_force = Eigen::Vector3f(-0.0068f, 0.0f, 0.0022f) * 1e-3f;
    Eigen::Vector3f expected_torque = Eigen::Vector3f(0.0f, 0.0036f, 0.0f) * 1e-4f;

    // Act
    sentman.calc_aero_force_and_torque(
        0.25f, normal, centroid, velocity, 300.0f, conditions, force, torque
    );

    // Assert
    EXPECT_TRUE(force.isApprox(expected_force, 1e-2f)) << "force [" << force.x() << ", " << force.y() << ", " << force.z() << "expected" << expected_force.x() <<", " << expected_force.y() << ", " << expected_force.z() << "]\n";
    EXPECT_TRUE(torque.isApprox(expected_torque, 1e-2f)) << "  Actual torque: [" << torque.x() << ", " << torque.y() << ", " << torque.z() << "]\n";
}

TEST_F(SentmanTest, CalcForceAndTorque_TemperatureRatioMethod12) {
    // Arrange
    Sentman sentman{ 1 };
    Eigen::Vector3f normal(1.0f, 0.0f, 0.0f);
    normal = normal.normalized();
    Eigen::Vector3f centroid(0.0f, 0.0f, 0.1666667f);
    Eigen::Vector3f velocity(7800.0f, 0.0f, 0.0f);
    auto conditions = create_leo_conditions();

    Eigen::Vector3f force;
    Eigen::Vector3f torque;
    Eigen::Vector3f expected_torque = Eigen::Vector3f(0.0f, -0.3857f, 0.0f) * 1e-4f;
    Eigen::Vector3f expected_force = Eigen::Vector3f(-0.2314f, 0.0f, 0.0f) * 1e-3f;

    // Act
    sentman.calc_aero_force_and_torque(
        0.25f, normal, centroid, velocity, 300.0f, conditions, force, torque
    );

    // Assert
    EXPECT_TRUE(force.isApprox(expected_force, 1e-2f)) << "  Actual force: [" << force.x() << ", " << force.y() << ", " << force.z() << "]\n";
    EXPECT_TRUE(torque.isApprox(expected_torque, 1e-2f)) << "  Actual torque: [" << torque.x() << ", " << torque.y() << ", " << torque.z() << "]\n";
}
TEST_F(SentmanTest, CalcForceAndTorque_TemperatureRatioMethod2) {
    // Arrange
    Sentman sentman{ 2 };
    Eigen::Vector3f normal(1.0f, 1.0f, 0.0f);
    normal = normal.normalized();
    Eigen::Vector3f centroid(1.0f, 1.0f, 1.0f);
    Eigen::Vector3f velocity(7800.0f, 0.0f, 0.0f);
    auto conditions = create_leo_conditions();

    Eigen::Vector3f force;
    Eigen::Vector3f torque;
    Eigen::Vector3f expected_torque = Eigen::Vector3f(0.0843f, 0.4526f, -0.5370f) * 1e-3f;
    Eigen::Vector3f expected_force = Eigen::Vector3f(0.4526f, -0.0843f, 0.0f) * 1e-3f;

    // Act
    sentman.calc_aero_force_and_torque(
        1.0f, normal, centroid, velocity, 300.0f, conditions, force, torque
    );

    // Assert
    EXPECT_TRUE(force.isApprox(expected_force, 1e-3f)) << "  Actual force: [" << force.x() << ", " << force.y() << ", " << force.z() << "]\n";
    EXPECT_TRUE(torque.isApprox(expected_torque, 1e-3f)) << "  Actual torque: [" << torque.x() << ", " << torque.y() << ", " << torque.z() << "]\n";
}

TEST_F(SentmanTest, CalcForceAndTorque_TemperatureRatioMethod3) {
    // Arrange
    Sentman sentman{ 3 };
    Eigen::Vector3f normal(1.0f, 1.0f, 0.0f);
    normal = normal.normalized();
    Eigen::Vector3f centroid(1.0f, 1.0f, 1.0f);
    Eigen::Vector3f velocity(7800.0f, 0.0f, 0.0f);
    auto conditions = create_leo_conditions();

    Eigen::Vector3f force;
    Eigen::Vector3f torque;
    Eigen::Vector3f expected_torque = Eigen::Vector3f(0.0829f, 0.4541f, -0.5370f) * 1e-3f;
    Eigen::Vector3f expected_force = Eigen::Vector3f(0.4541f, -0.0829f, 0.0f) * 1e-3f;

    // Act
    sentman.calc_aero_force_and_torque(
        1.0f, normal, centroid, velocity, 300.0f, conditions, force, torque
    );

    // Assert
    EXPECT_TRUE(force.isApprox(expected_force, 1e-3f)) << "  Actual force: [" << force.x() << ", " << force.y() << ", " << force.z() << "]\n";
    EXPECT_TRUE(torque.isApprox(expected_torque, 1e-3f)) << "  Actual torque: [" << torque.x() << ", " << torque.y() << ", " << torque.z() << "]\n";
}