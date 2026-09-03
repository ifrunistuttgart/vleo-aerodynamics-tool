#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include "gsi.h"
#include "core.h"

// Pins down the sign of the tangential (shear) term in the lift coefficient for the
// models that decompose into cp/ctau:
//
//     cd = cp * cos_d + ctau * sin_d
//     cl = cp * sin_d - ctau * cos_d      <-- the minus is what these tests guard
//
// Shear acts downstream of the surface, so it must REDUCE lift. Rather than pinning
// absolute values (which would just re-encode whatever the code currently does), each
// test evaluates the same geometry twice -- once with sigma_t = 0 and once with
// sigma_t = 1 -- and asserts that adding shear moves lift down. A flipped sign makes
// lift go up instead, and the test fails.

namespace {

// 45 deg incidence: the shear term carries a full cos_d factor here, so its
// contribution to lift is at its most visible.
constexpr float V__m_per_s = 7800.0f;
constexpr float AREA__M2 = 1.0f;
constexpr float SURF_TEMP__K = 300.0f;

AeroConditions leo_conditions() {
    AeroConditions cond;
    cond.density__kg_per_m3 = 1.2482e-11f;
    cond.T_atmospheric__K = 934.0f;
    cond.particle_mass__kg = 16 * 1.6605390689252e-27f;
    return cond;
}

const glm::vec3 v_rel__m_per_s(V__m_per_s, 0.0f, 0.0f);
const glm::vec3 normal = glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f));
const glm::vec3 centroid__m(0.0f, 0.0f, 0.0f);

// Same basis the models build internally. For this geometry it evaluates to (0,-1,0),
// and it is orthogonal to drag_dir = -v_hat = (-1,0,0), so projecting the total force
// onto it isolates the lift contribution exactly.
glm::vec3 lift_direction() {
    return -glm::normalize(glm::cross(glm::cross(v_rel__m_per_s, normal), v_rel__m_per_s));
}

float lift_component(IGSIModelCPU& model) {
    AeroConditions cond = leo_conditions();
    glm::vec3 force__N(0.0f);
    glm::vec3 torque__Nm(0.0f);
    model.calc_aero_force_and_torque(AREA__M2, normal, centroid__m, v_rel__m_per_s,
                                     SURF_TEMP__K, cond, force__N, torque__Nm);
    return glm::dot(force__N, lift_direction());
}

} // namespace

TEST(LiftSignTest, SchaafChambre_ShearReducesLift) {
    gsi::cpu::SchaafChambre no_shear(0.9f, 0.0f);
    gsi::cpu::SchaafChambre full_shear(0.9f, 1.0f);

    const float lift_no_shear = lift_component(no_shear);
    const float lift_full_shear = lift_component(full_shear);

    // Without shear, lift is purely cp * sin_d and must be positive -- this also
    // catches a wholesale sign flip of the lift direction.
    EXPECT_GT(lift_no_shear, 0.0f);

    // Adding shear must pull lift down, not push it up.
    EXPECT_LT(lift_full_shear, lift_no_shear);
}

TEST(LiftSignTest, Storch_ShearReducesLift) {
    gsi::cpu::Storch no_shear(100.0f, 0.9f, 0.0f);
    gsi::cpu::Storch full_shear(100.0f, 0.9f, 1.0f);

    const float lift_no_shear = lift_component(no_shear);
    const float lift_full_shear = lift_component(full_shear);

    EXPECT_GT(lift_no_shear, 0.0f);
    EXPECT_LT(lift_full_shear, lift_no_shear);
}
