#include "pch.h"
#include "sentman.h"
#include "core/core.h"
#include <glm/glm.hpp>


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
TEST_F(SentmanTest, CalcForceAndTorque_TemperatureRatioMethod11) {
    // Arrange
    Sentman sentman{ 1 };
    glm::vec3 normal(0.0f, 0.0f, -1.0f);
    normal = glm::normalize(normal);
    glm::vec3 centroid(-0.166667f, 0.0f, 0.0f);
    glm::vec3 velocity(7800.0f, 0.0f, 0.0f);
    auto conditions = create_leo_conditions();

    glm::vec3 force;
    glm::vec3 torque;
    glm::vec3 expected_force = glm::vec3(-0.0068f, 0.0f, 0.0022f) * 1e-3f;
    glm::vec3 expected_torque = glm::vec3(0.0f, 0.0036f, 0.0f) * 1e-4f;

    // Act
    sentman.calc_aero_force_and_torque(
        0.25f, normal, centroid, velocity, 300.0f, conditions, force, torque
    );

    // Assert
    EXPECT_NEAR(force.x, expected_force.x, 1e-2f);
    EXPECT_NEAR(force.y, expected_force.y, 1e-2f);
    EXPECT_NEAR(force.z, expected_force.z, 1e-2f);
    EXPECT_NEAR(torque.x, expected_torque.x, 1e-2f);
    EXPECT_NEAR(torque.y, expected_torque.y, 1e-2f);
    EXPECT_NEAR(torque.z, expected_torque.z, 1e-2f);
}

TEST_F(SentmanTest, CalcForceAndTorque_TemperatureRatioMethod12) {
    // Arrange
    Sentman sentman{ 1 };
    glm::vec3 normal(1.0f, 0.0f, 0.0f);
    normal = glm::normalize(normal);
    glm::vec3 centroid(0.0f, 0.0f, 0.1666667f);
    glm::vec3 velocity(7800.0f, 0.0f, 0.0f);
    auto conditions = create_leo_conditions();

    glm::vec3 force;
    glm::vec3 torque;
    glm::vec3 expected_torque = glm::vec3(0.0f, -0.3857f, 0.0f) * 1e-4f;
    glm::vec3 expected_force = glm::vec3(-0.2314f, 0.0f, 0.0f) * 1e-3f;

    // Act
    sentman.calc_aero_force_and_torque(
        0.25f, normal, centroid, velocity, 300.0f, conditions, force, torque
    );

    // Assert
    EXPECT_NEAR(force.x, expected_force.x, 1e-2f);
    EXPECT_NEAR(force.y, expected_force.y, 1e-2f);
    EXPECT_NEAR(force.z, expected_force.z, 1e-2f);
    EXPECT_NEAR(torque.x, expected_torque.x, 1e-2f);
    EXPECT_NEAR(torque.y, expected_torque.y, 1e-2f);
    EXPECT_NEAR(torque.z, expected_torque.z, 1e-2f);
}
TEST_F(SentmanTest, CalcForceAndTorque_TemperatureRatioMethod2) {
    // Arrange
    Sentman sentman{ 2 };
    glm::vec3 normal(1.0f, 1.0f, 0.0f);
    normal = glm::normalize(normal);
    glm::vec3 centroid(1.0f, 1.0f, 1.0f);
    glm::vec3 velocity(7800.0f, 0.0f, 0.0f);
    auto conditions = create_leo_conditions();

    glm::vec3 force;
    glm::vec3 torque;
    glm::vec3 expected_torque = glm::vec3(0.0843f, 0.4526f, -0.5370f) * 1e-3f;
    glm::vec3 expected_force = glm::vec3(0.4526f, -0.0843f, 0.0f) * 1e-3f;

    // Act
    sentman.calc_aero_force_and_torque(
        1.0f, normal, centroid, velocity, 300.0f, conditions, force, torque
    );

    // Assert
    EXPECT_NEAR(force.x, expected_force.x, 1e-3f);
    EXPECT_NEAR(force.y, expected_force.y, 1e-3f);
    EXPECT_NEAR(force.z, expected_force.z, 1e-3f);
    EXPECT_NEAR(torque.x, expected_torque.x, 1e-3f);
    EXPECT_NEAR(torque.y, expected_torque.y, 1e-3f);
    EXPECT_NEAR(torque.z, expected_torque.z, 1e-3f);
}

TEST_F(SentmanTest, CalcForceAndTorque_TemperatureRatioMethod3) {
    // Arrange
    Sentman sentman{ 3 };
    glm::vec3 normal(1.0f, 1.0f, 0.0f);
    normal = glm::normalize(normal);
    glm::vec3 centroid(1.0f, 1.0f, 1.0f);
    glm::vec3 velocity(7800.0f, 0.0f, 0.0f);
    auto conditions = create_leo_conditions();

    glm::vec3 force;
    glm::vec3 torque;
    glm::vec3 expected_torque = glm::vec3(0.0829f, 0.4541f, -0.5370f) * 1e-3f;
    glm::vec3 expected_force = glm::vec3(0.4541f, -0.0829f, 0.0f) * 1e-3f;

    // Act
    sentman.calc_aero_force_and_torque(
        1.0f, normal, centroid, velocity, 300.0f, conditions, force, torque
    );

    // Assert
    EXPECT_NEAR(force.x, expected_force.x, 1e-3f);
    EXPECT_NEAR(force.y, expected_force.y, 1e-3f);
    EXPECT_NEAR(force.z, expected_force.z, 1e-3f);
    EXPECT_NEAR(torque.x, expected_torque.x, 1e-3f);
    EXPECT_NEAR(torque.y, expected_torque.y, 1e-3f);
    EXPECT_NEAR(torque.z, expected_torque.z, 1e-3f);
}