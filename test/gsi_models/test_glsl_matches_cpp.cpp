#include <gtest/gtest.h>
#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include <glm/glm.hpp>

#include "compute_shader.h"
#include "gl_helpers.h"
#include "glfw_opengl_context.h"
#include "cook.h"
#include "maxwell.h"
#include "newton.h"
#include "schaaf_chambre.h"
#include "sentman.h"
#include "storch.h"
#include "core.h"

using namespace vat;
using namespace vat::gsi_models;
// The GLCall macro names these unqualified.
using vat::gl::GLClearError;
using vat::gl::GLLogCall;

// Every GSI model has two implementations of the same formula: force_per_area() in
// C++, used per triangle, and gsi_force_per_projected_area() in GLSL, used per pixel.
// This evaluates both on the same random inputs and requires
//   GLSL == C++ / cos(delta)
// to float precision.

namespace {

constexpr int NUM_SAMPLES = 4096;
constexpr float MIN_COS_DELTA = 1e-3f;

struct Sample {
    glm::vec4 normal_and_wall_temp; // xyz: unit normal, w: surface temperature [K]
    glm::vec4 v_rel;                // xyz: relative velocity [m/s]
};

const char* HARNESS_HEADER = R"GLSL(#version 430
layout(local_size_x = 64) in;
uniform float u_min_cos_delta;

struct Sample { vec4 normal_and_wall_temp; vec4 v_rel; };
layout(std430, binding = 0) readonly buffer Inputs { Sample samples[]; };
layout(std430, binding = 1) writeonly buffer Outputs { vec4 results[]; };
)GLSL";

const char* HARNESS_MAIN = R"GLSL(
void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= uint(samples.length()))
    {
        return;
    }
    Sample s = samples[i];
    results[i] = vec4(gsi_force_per_projected_area(s.normal_and_wall_temp.xyz, s.v_rel.xyz, s.normal_and_wall_temp.w), 0.0);
}
)GLSL";

// Random windward inputs, including grazing incidence.
std::vector<Sample> make_samples(std::mt19937& rng) {
    std::normal_distribution<float> gauss(0.0f, 1.0f);
    std::uniform_real_distribution<float> speed(6000.0f, 8500.0f);
    std::uniform_real_distribution<float> wall_temp(100.0f, 600.0f);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);

    std::vector<Sample> samples;
    while (samples.size() < NUM_SAMPLES) {
        const glm::vec3 n = glm::normalize(glm::vec3(gauss(rng), gauss(rng), gauss(rng)));
        glm::vec3 direction = glm::normalize(glm::vec3(gauss(rng), gauss(rng), gauss(rng)));
        if (samples.size() % 8 == 0) {
            // Grazing: tilt the flow to cos(delta) in [2e-3, 1e-2].
            const glm::vec3 tangent = glm::normalize(direction - glm::dot(direction, n) * n);
            const float cos_d = 2e-3f + 8e-3f * unit(rng);
            direction = cos_d * n + std::sqrt(1.0f - cos_d * cos_d) * tangent;
        }
        if (glm::dot(direction, n) <= MIN_COS_DELTA) {
            continue;
        }
        samples.push_back(Sample{glm::vec4(n, wall_temp(rng)), glm::vec4(direction * speed(rng), 0.0f)});
    }
    return samples;
}

std::vector<glm::vec4> run_glsl(const IGSIModel& model, const AeroConditions& aero, const std::vector<Sample>& samples) {
    gl::ComputeShader shader(std::string(HARNESS_HEADER) + model.glsl_force_per_projected_area() + HARNESS_MAIN, true);

    GLuint buffers[2];
    GLCall(glGenBuffers(2, buffers));
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[0]));
    GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(samples.size() * sizeof(Sample)), samples.data(), GL_STATIC_DRAW));
    GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffers[0]));
    GLCall(glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers[1]));
    GLCall(glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizeiptr>(samples.size() * sizeof(glm::vec4)), nullptr, GL_DYNAMIC_READ));
    GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, buffers[1]));

    shader.Bind();
    shader.SetUniform1f("u_min_cos_delta", MIN_COS_DELTA);
    for (const GlslUniform& uniform : model.glsl_uniforms(aero)) {
        std::visit([&](auto value) {
            if constexpr (std::is_same_v<decltype(value), int>) {
                shader.SetUniform1i(uniform.name, value);
            } else {
                shader.SetUniform1f(uniform.name, value);
            }
        }, uniform.value);
    }
    GLCall(glDispatchCompute(static_cast<GLuint>((samples.size() + 63) / 64), 1, 1));
    GLCall(glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT));

    std::vector<glm::vec4> results(samples.size());
    GLCall(glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, static_cast<GLsizeiptr>(results.size() * sizeof(glm::vec4)), results.data()));
    shader.Unbind();
    GLCall(glDeleteBuffers(2, buffers));
    return results;
}

struct ModelCase {
    std::string name;
    std::function<std::unique_ptr<IGSIModel>()> make;
};

class GlslMatchesCpp : public ::testing::TestWithParam<ModelCase> {
protected:
    static void SetUpTestSuite() {
        s_context = std::make_unique<gl::GlfwOpenGLContext>(64, 64, "GLSL harness", false);
    }
    static void TearDownTestSuite() {
        s_context.reset();
    }
    static std::unique_ptr<gl::GlfwOpenGLContext> s_context;
};

std::unique_ptr<gl::GlfwOpenGLContext> GlslMatchesCpp::s_context;

} // namespace

TEST_P(GlslMatchesCpp, OnRandomWindwardInputs) {
    const std::unique_ptr<IGSIModel> model = GetParam().make();
    if (model->glsl_force_per_projected_area().empty()) {
        GTEST_SKIP() << GetParam().name << " has no GPU implementation yet";
    }
    s_context->make_current();

    std::mt19937 rng(20260924);
    // Atomic oxygen and molecular nitrogen, thin and dense, cold and hot.
    const std::vector<AeroConditions> atmospheres{
        {1.2482e-11f, 934.0f, 16 * 1.6605390689252e-27f},
        {3.0e-13f, 1400.0f, 16 * 1.6605390689252e-27f},
        {5.0e-10f, 600.0f, 28 * 1.6605390689252e-27f},
    };

    for (const AeroConditions& aero : atmospheres) {
        const std::vector<Sample> samples = make_samples(rng);
        const std::vector<glm::vec4> gpu = run_glsl(*model, aero, samples);

        double max_error = 0.0;
        for (size_t i = 0; i < samples.size(); ++i) {
            const glm::vec3 n(samples[i].normal_and_wall_temp);
            const float wall_temp = samples[i].normal_and_wall_temp.w;
            const glm::vec3 v(samples[i].v_rel);
            const double cos_d = glm::dot(glm::dvec3(n), glm::dvec3(v)) / glm::length(glm::dvec3(v));
            const glm::dvec3 expected = glm::dvec3(model->force_per_area(n, v, wall_temp, aero)) / cos_d;

            // Relative to rho*v^2 per projected area, the natural scale of every model,
            // so results close to zero are judged on the same footing as large ones.
            const double scale = aero.density__kg_per_m3 * glm::dot(glm::dvec3(v), glm::dvec3(v)) / cos_d;
            const double error = glm::length(glm::dvec3(gpu[i]) - expected) / scale;
            max_error = std::max(max_error, error);
            ASSERT_LT(error, 1e-5) << GetParam().name << " sample " << i
                << " n=(" << n.x << ", " << n.y << ", " << n.z << ") v=(" << v.x << ", " << v.y << ", " << v.z
                << ") Tw=" << wall_temp << " cos_d=" << cos_d;
        }
        std::printf("[          ] %s: max relative error %.3e over %d samples\n", GetParam().name.c_str(), max_error, NUM_SAMPLES);
    }
}

INSTANTIATE_TEST_SUITE_P(AllModels, GlslMatchesCpp, ::testing::Values(
    ModelCase{"Newton", [] { return std::make_unique<Newton>(); }},
    ModelCase{"Cook", [] { return std::make_unique<Cook>(0.9f); }},
    ModelCase{"Maxwell", [] { return std::make_unique<Maxwell>(0.9f); }},
    ModelCase{"SchaafChambre", [] { return std::make_unique<SchaafChambre>(0.9f, 0.9f); }},
    ModelCase{"Storch", [] { return std::make_unique<Storch>(500.0f, 0.9f, 0.9f); }},
    ModelCase{"Sentman1", [] { return std::make_unique<Sentman>(1, 0.9f); }},
    ModelCase{"Sentman2", [] { return std::make_unique<Sentman>(2, 0.9f); }},
    ModelCase{"Sentman3", [] { return std::make_unique<Sentman>(3, 0.9f); }}
), [](const ::testing::TestParamInfo<ModelCase>& info) { return info.param.name; });
