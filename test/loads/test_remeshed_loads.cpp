#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>

#include <glm/glm.hpp>

#include "core.h"
#include "hybrid_aero_load_calculator.h"
#include "pixel_sizing.h"
#include "remesh.h"
#include "sentman.h"
#include "shading_pipeline.h"
#include "static_mesh_geometry.h"

using namespace vat;

namespace {

std::string Shuttlecock() {
    return (std::filesystem::path(__FILE__).parent_path()
        / "../../matlab/examples/geometries/shuttlecock_960.obj").string();
}

// Includes directions that put the wings in each other's shadow, where sliver
// triangles across shadow edges hurt most.
const std::array<glm::vec3, 4> FLOWS{
    glm::vec3(7800.0f, 0.0f, 0.0f),
    glm::vec3(7000.0f, 3000.0f, 1500.0f),
    glm::vec3(-3000.0f, 1000.0f, 7000.0f),
    glm::vec3(2000.0f, -7000.0f, 3000.0f),
};

struct Loads {
    std::array<glm::vec3, FLOWS.size()> force__N;
    std::array<glm::vec3, FLOWS.size()> torque__Nm;
};

// num_pixel 0 means: let the pipeline choose.
Loads Evaluate(IGeometryShadingData& geometry, unsigned int num_pixel) {
    gsi_models::Sentman sentman(1, 0.9f);
    AeroConditions aero{1.2482e-11f, 934.0f, 16 * 1.6605390689252e-27f};
    auto pipeline = num_pixel == 0
        ? std::make_unique<shading::ShadingPipeline>(geometry, shading::ShadingAlgorithmType::CoP)
        : std::make_unique<shading::ShadingPipeline>(geometry, shading::ShadingAlgorithmType::CoP, num_pixel);
    loads::HybridForceTorqueCalculator calculator(geometry, *pipeline, sentman);
    Loads out{};
    for (std::size_t i = 0; i < FLOWS.size(); ++i) {
        calculator.calc_aero_torque_force(FLOWS[i], 300.0f, aero, out.torque__Nm[i], out.force__N[i]);
    }
    return out;
}

// Worst force difference over all flows, relative to the largest reference force.
float WorstForceError(const Loads& loads, const Loads& reference) {
    float scale = 0.0f, worst = 0.0f;
    for (std::size_t i = 0; i < FLOWS.size(); ++i) {
        scale = std::max(scale, glm::length(reference.force__N[i]));
        worst = std::max(worst, glm::length(loads.force__N[i] - reference.force__N[i]));
    }
    return worst / scale;
}

} // namespace

// The end-to-end claim of remeshing plus automatic num_pixel: a modest remeshed mesh
// at the pipeline's own num_pixel reproduces a much finer reference, and does so far
// better than the original sliver mesh at a generous num_pixel.
TEST(RemeshedLoadsTest, RemeshedMeshAtAutomaticNumPixelMatchesAFineReference) {
    geometry::StaticMeshGeometry original(Shuttlecock());

    remeshing::RemeshOptions fine;
    fine.target_triangle_count = 80000;
    remeshing::RemeshResult reference_mesh = remeshing::remesh(original, fine);
    const Loads reference = Evaluate(*reference_mesh.geometry, 6400);

    remeshing::RemeshOptions modest;
    modest.target_triangle_count = 20000;
    remeshing::RemeshResult remeshed = remeshing::remesh(original, modest);
    const float remeshed_error = WorstForceError(Evaluate(*remeshed.geometry, 0), reference);

    const float original_error = WorstForceError(Evaluate(original, 4000), reference);

    std::printf("worst force error vs reference: remeshed at num_pixel %u: %.3f %%, original at 4000: %.3f %%\n",
        shading::suggest_num_pixel(*remeshed.geometry, shading::ShadingAlgorithmType::CoP),
        100.0f * remeshed_error, 100.0f * original_error);

    // Measured: 0.03 % for the remeshed mesh, 2.7 % for the original. The bounds leave
    // room for other GPUs rasterising edge cases differently.
    EXPECT_LT(remeshed_error, 0.01f);
    EXPECT_LT(remeshed_error, original_error / 3.0f);
}
