#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <cmath>
#include "projected_area.h"
#include "core.h"

using namespace vat;
using namespace vat::gsi_models;

namespace {

glm::vec3 force_of(const glm::vec3& normal, const glm::vec3& v_rel, float area) {
    ProjectedArea model;
    AeroConditions cond{};
    glm::vec3 force(0.0f), torque(0.0f);
    model.calc_aero_force_and_torque(area, normal, glm::vec3(1.0f, 2.0f, 3.0f), v_rel, 300.0f, cond, force, torque);
    return force;
}

} // namespace

TEST(ProjectedAreaTest, FaceOnGivesFullArea) {
    const glm::vec3 f = force_of(glm::vec3(1, 0, 0), glm::vec3(7800, 0, 0), 2.5f);
    EXPECT_FLOAT_EQ(f.x, 2.5f);
    EXPECT_FLOAT_EQ(f.y, 0.0f);
    EXPECT_FLOAT_EQ(f.z, 0.0f);
}

TEST(ProjectedAreaTest, TiltedGivesCosineOfArea) {
    const glm::vec3 n = glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));
    const glm::vec3 f = force_of(n, glm::vec3(7800, 0, 0), 2.0f);
    EXPECT_NEAR(f.x, 2.0f * std::sqrt(0.5f), 1e-6f);
}

TEST(ProjectedAreaTest, LeewardAndEdgeOnGiveZero) {
    EXPECT_EQ(force_of(glm::vec3(-1, 0, 0), glm::vec3(7800, 0, 0), 1.0f), glm::vec3(0.0f));
    EXPECT_EQ(force_of(glm::vec3(0, 1, 0), glm::vec3(7800, 0, 0), 1.0f), glm::vec3(0.0f));
}

TEST(ProjectedAreaTest, IndependentOfFlowSpeed) {
    const glm::vec3 n(1, 0, 0);
    EXPECT_FLOAT_EQ(force_of(n, glm::vec3(100, 0, 0), 1.0f).x, force_of(n, glm::vec3(9000, 0, 0), 1.0f).x);
}
