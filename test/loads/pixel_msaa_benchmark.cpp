#define FMT_UNICODE 0 // avoid error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "gsi.h"
#include "hybrid_aero_load_calculator.h"
#include "pixel_force_torque_calculator.h"
#include "core.h"
#include "shading_pipeline.h"
#include "shading_algorithm_factory.h"
#include "in_memory_geometry.h"

// Accuracy against Hybrid (exact on convex bodies) and runtime of 1 sample vs 8x MSAA, as
// CSV lines. Not a test; run it explicitly with
//   --gtest_filter=*MsaaBenchmark* --gtest_also_run_disabled_tests

using namespace vat;
using namespace vat::gsi_models;
using namespace vat::shading;
using namespace vat::loads;

namespace {

constexpr float SURFACE_TEMP__K = 300.0f;
constexpr float SPEED__M_PER_S = 7800.0f;
constexpr unsigned int SAMPLE_COUNTS[] = {1u, 8u};
constexpr unsigned int RESOLUTIONS[] = {256u, 512u, 1024u, 2000u};

AeroConditions leo_conditions() {
    return AeroConditions{1.2482e-11f, 934.0f, 16 * 1.6605390689252e-27f};
}

struct Loads {
    glm::dvec3 force__N;
    glm::dvec3 torque__Nm;
};

Loads evaluate(IAeroLoadCalculator& calculator, const glm::vec3& v) {
    AeroConditions aero = leo_conditions();
    glm::vec3 force, torque;
    calculator.calc_aero_torque_force(v, SURFACE_TEMP__K, aero, torque, force);
    return {glm::dvec3(force), glm::dvec3(torque)};
}

PixelLoadOptions options_with(unsigned int samples) {
    PixelLoadOptions options;
    options.num_msaa_samples = samples;
    return options;
}

} // namespace

TEST(PixelMsaaBenchmark, DISABLED_MsaaBenchmark) {
    Newton newton;
    Sentman sentman(1, 0.9f);
    InMemoryGeometry box;
    box.add_box({-0.3f, -0.2f, -0.1f}, {0.5f, 0.4f, 0.3f});
    InMemoryGeometry sphere;
    sphere.add_sphere(0.5f, 4);
    const std::vector<std::pair<const char*, IGeometryShadingData*>> bodies{{"box", &box}, {"sphere", &sphere}};
    const std::vector<std::pair<const char*, IGSIModel*>> models{{"Newton", &newton}, {"Sentman", &sentman}};
    const std::vector<glm::vec3> directions{
        glm::normalize(glm::vec3(1.0f, 0.3f, -0.2f)),
        glm::normalize(glm::vec3(-0.4f, 0.8f, 0.5f)),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::normalize(glm::vec3(-0.2f, -0.5f, -1.0f)),
    };

    std::printf("ACCURACY body,model,N,samples,max_force_err,max_torque_err\n");
    for (const auto& [body_name, geometry] : bodies) {
        const double radius = geometry->get_bounding_sphere_radius();
        ShadingPipeline pipeline(*geometry, ShadingAlgorithmType::Binary, 2048);
        for (const auto& [model_name, model] : models) {
            HybridForceTorqueCalculator hybrid(*geometry, pipeline, *model);
            std::vector<Loads> references;
            for (const glm::vec3& direction : directions) {
                references.push_back(evaluate(hybrid, direction * SPEED__M_PER_S));
            }
            for (unsigned int num_pixel : RESOLUTIONS) {
                for (unsigned int samples : SAMPLE_COUNTS) {
                    PixelForceTorqueCalculator pixel(*geometry, *model, num_pixel, options_with(samples));
                    double max_force = 0.0, max_torque = 0.0;
                    for (size_t d = 0; d < directions.size(); ++d) {
                        const Loads loads = evaluate(pixel, directions[d] * SPEED__M_PER_S);
                        const double scale = glm::length(references[d].force__N);
                        max_force = std::max(max_force, glm::length(loads.force__N - references[d].force__N) / scale);
                        max_torque = std::max(max_torque, glm::length(loads.torque__Nm - references[d].torque__Nm) / (scale * radius));
                    }
                    std::printf("ACCURACY %s,%s,%u,%u,%.3e,%.3e\n", body_name, model_name, num_pixel,
                        pixel.num_msaa_samples(), max_force, max_torque);
                }
            }
        }
    }

    // Median wall time of one call, which waits for the read-back, so it covers the whole GPU
    // work. Sample counts interleaved per repetition so clock drift hits both alike.
    std::printf("RUNTIME body,model,N,samples,median_ms\n");
    using Clock = std::chrono::steady_clock;
    const glm::vec3 v = directions[0] * SPEED__M_PER_S;
    for (const auto& [body_name, geometry] : bodies) {
        for (const auto& [model_name, model] : models) {
            for (unsigned int num_pixel : RESOLUTIONS) {
                std::vector<std::unique_ptr<PixelForceTorqueCalculator>> calculators;
                for (unsigned int samples : SAMPLE_COUNTS) {
                    calculators.push_back(std::make_unique<PixelForceTorqueCalculator>(*geometry, *model, num_pixel, options_with(samples)));
                    for (int warm_up = 0; warm_up < 5; ++warm_up) {
                        evaluate(*calculators.back(), v);
                    }
                }
                std::vector<std::vector<double>> times(calculators.size());
                for (int repetition = 0; repetition < 40; ++repetition) {
                    for (size_t c = 0; c < calculators.size(); ++c) {
                        const auto start = Clock::now();
                        evaluate(*calculators[c], v);
                        times[c].push_back(std::chrono::duration<double, std::milli>(Clock::now() - start).count());
                    }
                }
                for (size_t c = 0; c < calculators.size(); ++c) {
                    std::sort(times[c].begin(), times[c].end());
                    std::printf("RUNTIME %s,%s,%u,%u,%.3f\n", body_name, model_name, num_pixel,
                        calculators[c]->num_msaa_samples(), times[c][times[c].size() / 2]);
                }
            }
        }
    }
}
