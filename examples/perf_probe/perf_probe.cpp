// Development tool for the aero-load pipeline. Two modes:
//
//   perf_probe <mesh.obj> --fingerprint   deterministic results, for before/after diffing
//   perf_probe <mesh.obj> --timings [P..] per-phase timings
//
// The fingerprint matrix deliberately contains both an unrotated and a rotated
// configuration. Correctness work on the rotation path may change the rotated
// lines; the unrotated lines must never change.
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#define FMT_UNICODE 0
#include <spdlog/spdlog.h>

#include "sentman.h"
#include "rotatable_mesh_satellite.h"
#include "shading_pipeline.h"
#include "shading_algorithm_factory.h"
#include "hybrid_aero_load_calculator.h"

namespace {

using clk = std::chrono::steady_clock;

constexpr float SURFACE_TEMP__K = 300.0f;
constexpr float SPEED__M_PER_S = 7800.0f;

// Hinge deliberately off the body origin, so the model matrix carries a
// non-zero translation column.
constexpr std::array<float, 3> HINGE_ORIGIN{-0.15f, 0.0f, 0.05f};
constexpr std::array<float, 3> HINGE_AXIS{0.0f, 0.0f, -1.0f};

AeroConditions make_conditions() {
    return AeroConditions{1.2482e-11f, 934.0f, 16 * 1.6605390689252e-27f, 0.9f};
}

const std::vector<glm::vec3>& flow_directions() {
    static const std::vector<glm::vec3> dirs{
        glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f)),
        glm::normalize(glm::vec3(0.3f, 0.9f, 0.1f)),
        glm::normalize(glm::vec3(-0.5f, 0.2f, 0.8f)),
        glm::normalize(glm::vec3(0.0f, -1.0f, 0.4f)),
    };
    return dirs;
}

std::uint64_t hash_visibility(const std::vector<float>& v) {
    std::uint64_t h = 1469598103934665603ull; // FNV-1a
    for (float f : v) {
        const auto byte = static_cast<unsigned char>(f > 0.5f ? 1 : 0);
        h = (h ^ byte) * 1099511628211ull;
    }
    return h;
}

template <typename F>
double time_ms(int reps, F&& f) {
    f(); // warm-up, not measured
    std::vector<double> t;
    t.reserve(reps);
    for (int i = 0; i < reps; ++i) {
        const auto t0 = clk::now();
        f();
        const auto t1 = clk::now();
        t.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
    std::sort(t.begin(), t.end());
    return t[t.size() / 2];
}

void run_fingerprint(RotatableMeshSatellite& sat) {
    AeroConditions aero = make_conditions();
    Sentman gsi(1);

    std::printf("# rot alg    P  dir   visible                 hash          Fx            Fy            Fz"
                "            Tx            Ty            Tz\n");
    for (int rotated = 0; rotated <= 1; ++rotated) {
        for (unsigned int P : {512u, 1024u}) {
            for (int alg = 0; alg <= 1; ++alg) {
                // Set the pose before the pipeline is built, mirroring soar_rotatable.m.
                sat.turn_surface_around_axis(0, rotated ? 0.785398163f : 0.0f, HINGE_ORIGIN, HINGE_AXIS);

                const auto type = alg == 0 ? ShadingAlgorithmType::Binary : ShadingAlgorithmType::CoP;
                ShadingPipeline pipeline(sat, type, P);
                HybridForceTorqueCalculator calc(sat, pipeline, gsi);

                for (std::size_t d = 0; d < flow_directions().size(); ++d) {
                    const glm::vec3 v = flow_directions()[d] * SPEED__M_PER_S;
                    const std::vector<float> vis = pipeline.shade(glm::normalize(v));
                    const auto visible = std::count_if(vis.begin(), vis.end(), [](float f) { return f > 0.5f; });

                    glm::vec3 force__N, torque__Nm;
                    calc.calc_aero_torque_force(v, SURFACE_TEMP__K, aero, torque__Nm, force__N);

                    std::printf("  %d   %c %4u %4zu %9lld %20llu %13.6e %13.6e %13.6e %13.6e %13.6e %13.6e\n",
                                rotated, alg == 0 ? 'B' : 'C', P, d,
                                static_cast<long long>(visible),
                                static_cast<unsigned long long>(hash_visibility(vis)),
                                force__N.x, force__N.y, force__N.z,
                                torque__Nm.x, torque__Nm.y, torque__Nm.z);
                }
            }
        }
    }
    sat.turn_surface_around_axis(0, 0.0f, HINGE_ORIGIN, HINGE_AXIS);
}

void run_timings(RotatableMeshSatellite& sat, const std::vector<unsigned int>& resolutions) {
    const unsigned int N = sat.get_num_triangles();
    AeroConditions aero = make_conditions();
    Sentman gsi(1);
    const glm::vec3 v(SPEED__M_PER_S, 0.0f, 0.0f);
    const glm::vec3 vhat = glm::normalize(v);

    std::printf("[A] CPU geometry re-transform, per getter call\n");
    const double tN = time_ms(20, [&] { auto s = sat.get_normals(); (void)s.size(); });
    const double tC = time_ms(20, [&] { auto s = sat.get_centroids(); (void)s.size(); });
    const double tB = time_ms(20, [&] { volatile float r = sat.get_bounding_sphere_radius(); (void)r; });
    std::printf("    get_normals()                %8.3f ms\n", tN);
    std::printf("    get_centroids()              %8.3f ms\n", tC);
    std::printf("    get_bounding_sphere_radius() %8.3f ms\n", tB);
    std::printf("    -> per force/torque call     %8.3f ms\n\n", tN + tC + tB);

    auto areas = sat.get_areas();
    auto normals = sat.get_normals();
    auto centroids = sat.get_centroids();
    std::printf("[B] Sentman x %u triangles       %8.3f ms\n\n", N, time_ms(10, [&] {
        glm::vec3 f, tq;
        for (unsigned int i = 0; i < N; ++i) {
            const glm::vec3 n{normals[3 * i], normals[3 * i + 1], normals[3 * i + 2]};
            const glm::vec3 c{centroids[3 * i], centroids[3 * i + 1], centroids[3 * i + 2]};
            gsi.calc_aero_force_and_torque(areas[i], n, c, v, SURFACE_TEMP__K, aero, f, tq);
        }
    }));

    std::printf("[C] shade() and end-to-end\n");
    std::printf("    %6s %11s %11s %11s %11s\n", "P", "CoP", "Binary", "full", "full-shade");
    for (unsigned int P : resolutions) {
        const int reps = P >= 4000 ? 3 : 5;
        double t_cop, t_binary, t_full;
        {
            ShadingPipeline pipeline(sat, ShadingAlgorithmType::CoP, P);
            t_cop = time_ms(reps, [&] { auto r = pipeline.shade(vhat); (void)r.size(); });
            HybridForceTorqueCalculator calc(sat, pipeline, gsi);
            glm::vec3 f, tq;
            t_full = time_ms(reps, [&] { calc.calc_aero_torque_force(v, SURFACE_TEMP__K, aero, tq, f); });
        }
        {
            ShadingPipeline pipeline(sat, ShadingAlgorithmType::Binary, P);
            t_binary = time_ms(reps, [&] { auto r = pipeline.shade(vhat); (void)r.size(); });
        }
        std::printf("    %6u %11.3f %11.3f %11.3f %11.3f\n", P, t_cop, t_binary, t_full, t_full - t_cop);
    }
    std::printf("\n");
}

} // namespace

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    spdlog::set_level(spdlog::level::off);

    if (argc < 3) {
        std::printf("usage: perf_probe <mesh.obj> --fingerprint | --timings [P ...]\n");
        return 1;
    }
    const std::string mesh = argv[1];
    const std::string mode = argv[2];

    RotatableMeshSatellite sat(mesh);
    if (sat.get_num_triangles() == 0) {
        std::printf("mesh failed to load: %s\n", mesh.c_str());
        return 1;
    }
    std::printf("# %s : %u triangles\n", std::filesystem::path(mesh).filename().string().c_str(),
                sat.get_num_triangles());

    if (mode == "--fingerprint") {
        run_fingerprint(sat);
    } else if (mode == "--timings") {
        std::vector<unsigned int> resolutions;
        for (int i = 3; i < argc; ++i) resolutions.push_back(static_cast<unsigned int>(std::atoi(argv[i])));
        if (resolutions.empty()) resolutions = {1000u, 4000u};
        run_timings(sat, resolutions);
    } else {
        std::printf("unknown mode: %s\n", mode.c_str());
        return 1;
    }
    return 0;
}
