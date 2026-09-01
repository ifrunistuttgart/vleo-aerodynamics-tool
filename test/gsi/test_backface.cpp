#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include "cook.h"
#include "maxwell.h"
#include "newton.h"
#include "schaaf_chambre.h"
#include "storch.h"
#include "sentman.h"
#include "core.h"

// HybridForceTorqueCalculator hands EVERY triangle to the GSI model and assigns
// visibility = 1.0 to back-facing ones (hybrid_aero_load_calculator.cpp), relying on
// the model itself to return ~zero for a face turned away from the flow. A model that
// does not honour that contract contributes spurious load on every shadowed panel --
// for Newton and Storch it was spurious *thrust*, since cd = cp*cos_d goes negative
// once cos_d < 0.
//
// ADBSat guards this explicitly in each of its fmf_eq/coeff_*.m models
// (`ind = find(delta>pi/2); cp(ind) = 0; ctau(ind) = 0;`) rather than trusting the
// closed-form expressions to decay, and these tests hold our models to the same rule.

namespace {

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
const glm::vec3 centroid__m(0.0f, 0.0f, 0.0f);

// Tilted rather than exactly edge-on so sin_d != 0 and the shear terms are live too.
const glm::vec3 windward = glm::normalize(glm::vec3(1.0f, 0.4f, 0.0f));
const glm::vec3 leeward = -windward;

float force_magnitude(IGSIModel& model, const glm::vec3& normal) {
    AeroConditions cond = leo_conditions();
    glm::vec3 force__N(0.0f);
    glm::vec3 torque__Nm(0.0f);
    model.calc_aero_force_and_torque(AREA__M2, normal, centroid__m, v_rel__m_per_s,
                                     SURF_TEMP__K, cond, force__N, torque__Nm);
    return glm::length(force__N);
}

// A leeward panel must contribute negligibly next to the same panel facing the flow.
void expect_leeward_negligible(IGSIModel& model, const char* name) {
    const float windward_load = force_magnitude(model, windward);
    const float leeward_load = force_magnitude(model, leeward);

    ASSERT_GT(windward_load, 0.0f) << name << ": windward panel produced no load";
    EXPECT_LT(leeward_load, 1e-3f * windward_load)
        << name << ": leeward panel produced " << leeward_load
        << " N against a windward " << windward_load << " N";
}

} // namespace

TEST(BackFaceTest, Cook) { Cook m(0.9f); expect_leeward_negligible(m, "Cook"); }
TEST(BackFaceTest, Maxwell) { Maxwell m(0.9f); expect_leeward_negligible(m, "Maxwell"); }
TEST(BackFaceTest, Newton) { Newton m; expect_leeward_negligible(m, "Newton"); }
TEST(BackFaceTest, SchaafChambre) { SchaafChambre m(0.9f, 0.9f); expect_leeward_negligible(m, "SchaafChambre"); }
TEST(BackFaceTest, Storch) { Storch m(100.0f, 0.9f, 0.9f); expect_leeward_negligible(m, "Storch"); }

// Sentman carries no explicit guard: its erfc(-s*cos_delta) factor decays to zero on
// its own. Included as a control -- it should pass the same bar without one.
TEST(BackFaceTest, Sentman) { Sentman m(1, 0.9f); expect_leeward_negligible(m, "Sentman"); }
