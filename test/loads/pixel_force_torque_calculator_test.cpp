#define FMT_UNICODE 0 // avoid error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <memory>
#include <glm/glm.hpp>

#include "newton.h"
#include "sentman.h"
#include "hybrid_aero_load_calculator.h"
#include "pixel_force_torque_calculator.h"
#include "core.h"
#include "shading_pipeline.h"
#include "shading_algorithm_factory.h"
#include "static_mesh_geometry.h"

using namespace vat;
using namespace vat::gsi_models;
using namespace vat::geometry;
using namespace vat::shading;
using namespace vat::loads;

namespace {

std::string tetrahedron_path() {
    return (std::filesystem::path(__FILE__).parent_path() / "../geometries/tetraeder.obj").string();
}

AeroConditions leo_conditions() {
    return AeroConditions{1.2482e-11f, 934.0f, 16 * 1.6605390689252e-27f};
}

} // namespace

// Smoke tests for the end-to-end pipeline; accuracy is in pixel_force_torque_accuracy_test.cpp.
TEST(PixelForceTorqueCalculatorTest, NewtonTetrahedronAgreesWithHybrid) {
    StaticMeshGeometry geometry(tetrahedron_path());
    Newton newton;
    AeroConditions aero = leo_conditions();
    const glm::vec3 v_rel(7000.0f, 2000.0f, 3000.0f);

    ShadingPipeline pipeline(geometry, ShadingAlgorithmType::Binary, 1024);
    HybridForceTorqueCalculator hybrid(geometry, pipeline, newton);
    PixelForceTorqueCalculator pixel(geometry, newton, 1024);

    glm::vec3 force_hybrid, torque_hybrid, force_pixel, torque_pixel;
    hybrid.calc_aero_torque_force(v_rel, 300.0f, aero, torque_hybrid, force_hybrid);
    pixel.calc_aero_torque_force(v_rel, 300.0f, aero, torque_pixel, force_pixel);

    SPDLOG_INFO("[TEST] hybrid F=({}, {}, {}) T=({}, {}, {})", force_hybrid.x, force_hybrid.y, force_hybrid.z, torque_hybrid.x, torque_hybrid.y, torque_hybrid.z);
    SPDLOG_INFO("[TEST] pixel  F=({}, {}, {}) T=({}, {}, {})", force_pixel.x, force_pixel.y, force_pixel.z, torque_pixel.x, torque_pixel.y, torque_pixel.z);

    ASSERT_GT(glm::length(force_hybrid), 0.0f);
    EXPECT_LT(glm::length(force_pixel - force_hybrid) / glm::length(force_hybrid), 1e-2f);
    EXPECT_LT(glm::length(torque_pixel - torque_hybrid), 1e-2f * glm::length(force_hybrid) * geometry.get_bounding_sphere_radius());
}

TEST(PixelForceTorqueCalculatorTest, RepeatedEvaluationIsBitIdentical) {
    StaticMeshGeometry geometry(tetrahedron_path());
    Newton newton;
    AeroConditions aero = leo_conditions();
    const glm::vec3 v_rel(7000.0f, 2000.0f, 3000.0f);
    PixelForceTorqueCalculator pixel(geometry, newton, 512);

    glm::vec3 force_1, torque_1, force_2, torque_2;
    pixel.calc_aero_torque_force(v_rel, 300.0f, aero, torque_1, force_1);
    pixel.calc_aero_torque_force(v_rel, 300.0f, aero, torque_2, force_2);
    EXPECT_EQ(force_1, force_2);
    EXPECT_EQ(torque_1, torque_2);
}

TEST(PixelForceTorqueCalculatorTest, PressureImageIsReadBackOnRequest) {
    StaticMeshGeometry geometry(tetrahedron_path());
    Newton newton;
    AeroConditions aero = leo_conditions();
    PixelForceTorqueCalculator pixel(geometry, newton, 256, PixelLoadOptions{1e-3f, true});

    glm::vec3 force, torque;
    pixel.calc_aero_torque_force(glm::vec3(7800.0f, 0.0f, 0.0f), 300.0f, aero, torque, force);
    ASSERT_EQ(pixel.pressure_image().size(), 256u * 256u);
    float max_pressure = 0.0f;
    for (float p : pixel.pressure_image()) max_pressure = std::max(max_pressure, p);
    // Newton at normal incidence: p = 2q = rho v^2. No surface is quite normal here, so only bound it.
    EXPECT_GT(max_pressure, 0.0f);
    EXPECT_LE(max_pressure, aero.density__kg_per_m3 * 7800.0f * 7800.0f * 1.0001f);
}

TEST(PixelForceTorqueCalculatorTest, RejectsModelWithoutGpuImplementation) {
    StaticMeshGeometry geometry(tetrahedron_path());
    Sentman sentman(1, 0.9f);
    EXPECT_THROW(PixelForceTorqueCalculator(geometry, sentman, 256), std::invalid_argument);
}
