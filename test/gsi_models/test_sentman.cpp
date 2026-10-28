#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include "sentman.h"
#include "core.h"

using namespace vat;
using namespace vat::gsi_models;


// Test Fixture
class SentmanTest : public ::testing::Test {
protected:
    // Helper: Create standard LEO conditions
    AeroConditions create_leo_conditions() {
        AeroConditions cond;
        cond.density__kg_per_m3 = 1.2482e-11f;
        cond.T_atmospheric__K = 934.0f;          // Exospheric temperature
        cond.particle_mass__kg = 16 * 1.6605390689252e-27f;       // Atomic oxygen mass
        return cond;
    }
};

// Constructor Tests
TEST_F(SentmanTest, Constructor_ValidMethods_Construct) {
    EXPECT_NO_THROW({ Sentman s(1, 0.9f); });
    EXPECT_NO_THROW({ Sentman s(2, 0.9f); });
    EXPECT_NO_THROW({ Sentman s(3, 0.9f); });
}
// Reference values: Sentman's formula in double precision, as in the legacy MATLAB
// implementation (calcAeroForceAndTorque.m); tolerances are about 0.1 % of the force.
TEST_F(SentmanTest, CalcForceAndTorque_TemperatureRatioMethod11) {
    // Arrange
    Sentman sentman{ 1, 0.9f };
    glm::vec3 normal(0.0f, 0.0f, -1.0f);
    normal = glm::normalize(normal);
    glm::vec3 centroid(-0.166667f, 0.0f, 0.0f);
    glm::vec3 velocity(7800.0f, 0.0f, 0.0f);
    auto conditions = create_leo_conditions();

    glm::vec3 force;
    glm::vec3 torque;
    glm::vec3 expected_force = glm::vec3(-6.7649f, 0.0f, 2.1786f) * 1e-6f;
    glm::vec3 expected_torque = glm::vec3(0.0f, 3.6311f, 0.0f) * 1e-7f;

    // Act
    sentman.calc_aero_force_and_torque(
        0.25f, normal, centroid, velocity, 300.0f, conditions, force, torque
    );

    // Assert
    EXPECT_NEAR(force.x, expected_force.x, 1e-9f);
    EXPECT_NEAR(force.y, expected_force.y, 1e-9f);
    EXPECT_NEAR(force.z, expected_force.z, 1e-9f);
    EXPECT_NEAR(torque.x, expected_torque.x, 1e-9f);
    EXPECT_NEAR(torque.y, expected_torque.y, 1e-9f);
    EXPECT_NEAR(torque.z, expected_torque.z, 1e-9f);
}

TEST_F(SentmanTest, CalcForceAndTorque_TemperatureRatioMethod12) {
    // Arrange
    Sentman sentman{ 1, 0.9f };
    glm::vec3 normal(1.0f, 0.0f, 0.0f);
    normal = glm::normalize(normal);
    glm::vec3 centroid(0.0f, 0.0f, 0.1666667f);
    glm::vec3 velocity(7800.0f, 0.0f, 0.0f);
    auto conditions = create_leo_conditions();

    glm::vec3 force;
    glm::vec3 torque;
    glm::vec3 expected_torque = glm::vec3(0.0f, -3.8566f, 0.0f) * 1e-5f;
    glm::vec3 expected_force = glm::vec3(-2.3140f, 0.0f, 0.0f) * 1e-4f;

    // Act
    sentman.calc_aero_force_and_torque(
        0.25f, normal, centroid, velocity, 300.0f, conditions, force, torque
    );

    // Assert
    EXPECT_NEAR(force.x, expected_force.x, 1e-8f);
    EXPECT_NEAR(force.y, expected_force.y, 1e-8f);
    EXPECT_NEAR(force.z, expected_force.z, 1e-8f);
    EXPECT_NEAR(torque.x, expected_torque.x, 1e-8f);
    EXPECT_NEAR(torque.y, expected_torque.y, 1e-8f);
    EXPECT_NEAR(torque.z, expected_torque.z, 1e-8f);
}
TEST_F(SentmanTest, CalcForceAndTorque_TemperatureRatioMethod2) {
    // Arrange
    Sentman sentman{ 2, 0.9f };
    glm::vec3 normal(1.0f, 1.0f, 0.0f);
    normal = glm::normalize(normal);
    glm::vec3 centroid(1.0f, 1.0f, 1.0f);
    glm::vec3 velocity(7800.0f, 0.0f, 0.0f);
    auto conditions = create_leo_conditions();

    glm::vec3 force;
    glm::vec3 torque;
    glm::vec3 expected_torque = glm::vec3(0.8435f, -6.2133f, 5.3698f) * 1e-4f;
    glm::vec3 expected_force = glm::vec3(-6.2133f, -0.8435f, 0.0f) * 1e-4f;

    // Act
    sentman.calc_aero_force_and_torque(
        1.0f, normal, centroid, velocity, 300.0f, conditions, force, torque
    );

    // Assert
    EXPECT_NEAR(force.x, expected_force.x, 1e-7f);
    EXPECT_NEAR(force.y, expected_force.y, 1e-7f);
    EXPECT_NEAR(force.z, expected_force.z, 1e-7f);
    EXPECT_NEAR(torque.x, expected_torque.x, 1e-7f);
    EXPECT_NEAR(torque.y, expected_torque.y, 1e-7f);
    EXPECT_NEAR(torque.z, expected_torque.z, 1e-7f);
}

TEST_F(SentmanTest, CalcForceAndTorque_TemperatureRatioMethod3) {
    // Arrange
    Sentman sentman{ 3, 0.9f };
    glm::vec3 normal(1.0f, 1.0f, 0.0f);
    normal = glm::normalize(normal);
    glm::vec3 centroid(1.0f, 1.0f, 1.0f);
    glm::vec3 velocity(7800.0f, 0.0f, 0.0f);
    auto conditions = create_leo_conditions();

    glm::vec3 force;
    glm::vec3 torque;
    glm::vec3 expected_torque = glm::vec3(0.8292f, -6.1990f, 5.3698f) * 1e-4f;
    glm::vec3 expected_force = glm::vec3(-6.1990f, -0.8292f, 0.0f) * 1e-4f;

    // Act
    sentman.calc_aero_force_and_torque(
        1.0f, normal, centroid, velocity, 300.0f, conditions, force, torque
    );

    // Assert
    EXPECT_NEAR(force.x, expected_force.x, 1e-7f);
    EXPECT_NEAR(force.y, expected_force.y, 1e-7f);
    EXPECT_NEAR(force.z, expected_force.z, 1e-7f);
    EXPECT_NEAR(torque.x, expected_torque.x, 1e-7f);
    EXPECT_NEAR(torque.y, expected_torque.y, 1e-7f);
    EXPECT_NEAR(torque.z, expected_torque.z, 1e-7f);
}