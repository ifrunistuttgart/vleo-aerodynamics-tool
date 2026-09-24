#define FMT_UNICODE 0 // avoid error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <vector>
#include <glm/glm.hpp>

#include "newton.h"
#include "hybrid_aero_load_calculator.h"
#include "pixel_force_torque_calculator.h"
#include "core.h"
#include "shading_pipeline.h"
#include "shading_algorithm_factory.h"
#include "rotatable_mesh_geometry.h"
#include "in_memory_geometry.h"

using namespace vat;
using namespace vat::gsi_models;
using namespace vat::geometry;
using namespace vat::shading;
using namespace vat::loads;

// Accuracy of PixelForceTorqueCalculator, all with the Newton model, whose force per
// wetted area -rho*v^2*cos^2(delta)*n makes the analytic references short.

namespace {

constexpr float SURFACE_TEMP__K = 300.0f;
constexpr float SPEED__M_PER_S = 7800.0f;

AeroConditions leo_conditions() {
    return AeroConditions{1.2482e-11f, 934.0f, 16 * 1.6605390689252e-27f};
}

struct Loads {
    glm::dvec3 force__N;
    glm::dvec3 torque__Nm;
};

Loads evaluate(IAeroLoadCalculator& calculator, const glm::vec3& v_rel__m_per_s) {
    AeroConditions aero = leo_conditions();
    glm::vec3 force, torque;
    calculator.calc_aero_torque_force(v_rel__m_per_s, SURFACE_TEMP__K, aero, torque, force);
    return {glm::dvec3(force), glm::dvec3(torque)};
}

// Force error relative to |F_ref|; torque error relative to |F_ref| * R, the largest
// torque a force of that size can have inside the bounding sphere.
struct Errors {
    double force;
    double torque;
};

Errors relative_errors(const Loads& value, const Loads& reference, double radius) {
    const double scale = glm::length(reference.force__N);
    return {glm::length(value.force__N - reference.force__N) / scale,
            glm::length(value.torque__Nm - reference.torque__Nm) / (scale * radius)};
}

// Two single-sided plates facing +x. The front plate (x = 1) casts a shadow on the
// back plate (x = 0) for flow in the x-y plane at tan(alpha) = tan_alpha. The front
// plate also sticks out past the back plate, so the visible outline is not just the
// back plate and the torque depends on the shadow.
//   back:  y in [-1, 1],    z in [-1, 1]
//   front: y in [0.5, 1.5], z in [0, 1]
void add_two_plates(InMemoryGeometry& geometry, int back_cells = 1) {
    geometry.add_quad({0, -1, -1}, {0, 1, -1}, {0, 1, 1}, {0, -1, 1}, back_cells);
    geometry.add_quad({1, 0.5f, 0}, {1, 1.5f, 0}, {1, 1.5f, 1}, {1, 0.5f, 1});
}

glm::vec3 plate_flow(double tan_alpha) {
    return glm::normalize(glm::vec3(1.0f, static_cast<float>(tan_alpha), 0.0f)) * SPEED__M_PER_S;
}

struct PlateReference {
    Loads loads;
    double wetted_area;
};

PlateReference two_plates_reference(double tan_alpha) {
    // A point of the front plate shades the back plate at y - tan_alpha.
    const double overlap_y_lo = std::max(-1.0, 0.5 - tan_alpha);
    const double overlap_y_hi = std::min(1.0, 1.5 - tan_alpha);
    const double overlap_area = std::max(0.0, overlap_y_hi - overlap_y_lo) * 1.0;
    const double overlap_y_mid = 0.5 * (overlap_y_lo + overlap_y_hi);

    // Visible area and its first moments: back plate minus the shadow, plus the front.
    const double area = (4.0 - overlap_area) + 1.0;
    const double first_moment_y = (0.0 - overlap_area * overlap_y_mid) + 1.0 * 1.0;
    const double first_moment_z = (0.0 - overlap_area * 0.5) + 1.0 * 0.5;

    const AeroConditions aero = leo_conditions();
    const double cos2 = 1.0 / (1.0 + tan_alpha * tan_alpha);
    const double pressure = aero.density__kg_per_m3 * double(SPEED__M_PER_S) * SPEED__M_PER_S * cos2;
    // dF = -p dA x_hat, so r x dF = p dA (0, -z, y).
    return {{glm::dvec3(-pressure * area, 0.0, 0.0), glm::dvec3(0.0, -pressure * first_moment_z, pressure * first_moment_y)}, area};
}

// Pixels along an edge of total length L are each partly covered, so the covered area
// is off by at most L * h / 2 for pixel edge h, and the error falls like 1/N.
double edge_bound(double edge_length, double radius, unsigned int num_pixel, double area) {
    const double h = 2.0 * radius / num_pixel;
    return edge_length * h / 2.0 / area;
}

} // namespace

// (b) Without shadowing the pixel result must converge to Hybrid, which is exact
// there, at O(1/N).
TEST(PixelAccuracyTest, ConvexBodiesAgreeWithHybrid) {
    Newton newton;

    InMemoryGeometry box;
    box.add_box({-0.3f, -0.2f, -0.1f}, {0.5f, 0.4f, 0.3f}); // off-centre, so the torque is not zero
    InMemoryGeometry sphere;
    sphere.add_sphere(0.5f, 4); // 2048 triangles
    RotatableMeshGeometry tetrahedron(
        (std::filesystem::path(__FILE__).parent_path() / "../geometries/tetraeder.obj").string());
    // Hinge off the origin, so the model matrix carries a translation and the GPU
    // normal transform is exercised.
    tetrahedron.turn_mesh_around_axis(0, 0.6f, {0.1f, -0.2f, 0.05f}, {0.3f, 0.5f, 0.8f});

    const std::vector<std::pair<const char*, IGeometryShadingData*>> bodies{
        {"box", &box}, {"sphere", &sphere}, {"rotated tetrahedron", &tetrahedron}};
    const std::vector<glm::vec3> directions{
        glm::normalize(glm::vec3(1.0f, 0.3f, -0.2f)),
        glm::normalize(glm::vec3(-0.4f, 0.8f, 0.5f)),
        glm::vec3(0.0f, 1.0f, 0.0f), // camera up vector switches here
        glm::normalize(glm::vec3(-0.2f, -0.5f, -1.0f)),
    };

    for (const auto& [name, geometry] : bodies) {
        const double radius = geometry->get_bounding_sphere_radius();
        ShadingPipeline pipeline(*geometry, ShadingAlgorithmType::Binary, 2048);
        HybridForceTorqueCalculator hybrid(*geometry, pipeline, newton);

        for (unsigned int num_pixel : {256u, 512u, 1024u}) {
            PixelForceTorqueCalculator pixel(*geometry, newton, num_pixel);
            for (const glm::vec3& direction : directions) {
                const glm::vec3 v = direction * SPEED__M_PER_S;
                const Loads reference = evaluate(hybrid, v);
                const Errors e = relative_errors(evaluate(pixel, v), reference, radius);
                std::printf("[          ] %-20s N=%4u dir=(%5.2f %5.2f %5.2f) force %.2e torque %.2e\n",
                    name, num_pixel, direction.x, direction.y, direction.z, e.force, e.torque);
                // Edges that line up with the pixel grid (the box seen along +y) do not
                // average out their partly covered pixels, so the error there sits near
                // the worst case, about 2.7/N. Smooth outlines stay far below this.
                EXPECT_LT(e.force, 4.0 / num_pixel) << name;
                EXPECT_LT(e.torque, 4.0 / num_pixel) << name;
            }
        }
    }
}

// (c) The front plate shades part of the back plate. The pixel result must match the
// analytic visible area, force and moment, which Hybrid cannot: it either counts the
// partly shaded back plate fully or not at all.
TEST(PixelAccuracyTest, ShadowedPlatesMatchAnalyticAreaAndMoment) {
    Newton newton;
    InMemoryGeometry plates;
    add_two_plates(plates);
    const double radius = plates.get_bounding_sphere_radius();
    constexpr unsigned int N = 2048;
    constexpr double EDGES = 16.0; // total outline of visible regions, generously

    for (double tan_alpha : {0.0, 0.25}) {
        PixelForceTorqueCalculator pixel(plates, newton, N);
        const PlateReference reference = two_plates_reference(tan_alpha);
        const Loads loads = evaluate(pixel, plate_flow(tan_alpha));

        const double cos_alpha = 1.0 / std::sqrt(1.0 + tan_alpha * tan_alpha);
        const double bound = edge_bound(EDGES, radius, N, reference.wetted_area * cos_alpha);
        const Errors e = relative_errors(loads, reference.loads, radius);
        const double area_error = std::abs(pixel.last_wetted_area() - reference.wetted_area) / reference.wetted_area;
        std::printf("[          ] tan(alpha)=%.2f area %.2e force %.2e torque %.2e (bound %.2e)\n",
            tan_alpha, area_error, e.force, e.torque, bound);

        EXPECT_LT(area_error, bound);
        EXPECT_LT(e.force, bound);
        EXPECT_LT(e.torque, bound);
        EXPECT_NEAR(pixel.last_projected_area(), reference.wetted_area * cos_alpha, bound * reference.wetted_area);
    }
}

// (d) The pixel result depends on the resolution, not on the meshing.
TEST(PixelAccuracyTest, FlatPlateResultIsIndependentOfMeshing) {
    Newton newton;
    // A tilted parallelogram, so its outline is not aligned with the pixel grid.
    const glm::vec3 origin(0.1f, -0.4f, -0.3f), e1(0.1f, 0.8f, 0.1f), e2(-0.2f, 0.1f, 0.7f);
    InMemoryGeometry coarse, fine;
    coarse.add_quad(origin, origin + e1, origin + e1 + e2, origin + e2, 1);   // 2 triangles
    fine.add_quad(origin, origin + e1, origin + e1 + e2, origin + e2, 71);    // 10082 triangles
    ASSERT_EQ(coarse.get_num_triangles(), 2u);
    ASSERT_EQ(fine.get_num_triangles(), 10082u);
    ASSERT_EQ(coarse.get_bounding_sphere_radius(), fine.get_bounding_sphere_radius());

    const glm::vec3 v(7000.0f, 1500.0f, -2500.0f);
    constexpr unsigned int N = 1024;
    PixelForceTorqueCalculator pixel_coarse(coarse, newton, N);
    PixelForceTorqueCalculator pixel_fine(fine, newton, N);
    const Loads loads_coarse = evaluate(pixel_coarse, v);
    const Loads loads_fine = evaluate(pixel_fine, v);

    const double radius = coarse.get_bounding_sphere_radius();
    const Errors e = relative_errors(loads_fine, loads_coarse, radius);
    std::printf("[          ] 2 vs 10082 triangles: force %.2e torque %.2e\n", e.force, e.torque);
    EXPECT_LT(e.force, 1e-5);
    EXPECT_LT(e.torque, 1e-5);

    // Both also match the analytic uniform-pressure result.
    const glm::dvec3 cross = glm::cross(glm::dvec3(e1), glm::dvec3(e2));
    const double area = glm::length(cross);
    const glm::dvec3 n = cross / area;
    const double cos_d = glm::dot(n, glm::dvec3(v)) / glm::length(glm::dvec3(v));
    const glm::dvec3 force = -leo_conditions().density__kg_per_m3 * glm::dot(glm::dvec3(v), glm::dvec3(v)) * cos_d * cos_d * area * n;
    const glm::dvec3 centroid = glm::dvec3(origin) + 0.5 * glm::dvec3(e1 + e2);
    const Loads analytic{force, glm::cross(centroid, force)};
    const double perimeter = 2.0 * (glm::length(e1) + glm::length(e2));
    const double bound = edge_bound(perimeter, radius, N, area * cos_d);
    const Errors ea = relative_errors(loads_coarse, analytic, radius);
    std::printf("[          ] against analytic: force %.2e torque %.2e (bound %.2e)\n", ea.force, ea.torque, bound);
    EXPECT_LT(ea.force, bound);
    EXPECT_LT(ea.torque, bound);
}

// (d) continued: with shadowing, Hybrid depends on the meshing of the shaded plate
// and the pixel result does not.
TEST(PixelAccuracyTest, ShadowedPlatesResultIsIndependentOfMeshing) {
    Newton newton;
    InMemoryGeometry coarse, fine;
    add_two_plates(coarse, 1);
    add_two_plates(fine, 71);
    const glm::vec3 v = plate_flow(0.25);
    const double radius = coarse.get_bounding_sphere_radius();
    constexpr unsigned int N = 1024;

    PixelForceTorqueCalculator pixel_coarse(coarse, newton, N);
    PixelForceTorqueCalculator pixel_fine(fine, newton, N);
    const Errors e = relative_errors(evaluate(pixel_fine, v), evaluate(pixel_coarse, v), radius);
    std::printf("[          ] pixel,  coarse vs fine back plate: force %.2e torque %.2e\n", e.force, e.torque);
    EXPECT_LT(e.force, 1e-5);
    EXPECT_LT(e.torque, 1e-5);

    ShadingPipeline pipeline_coarse(coarse, ShadingAlgorithmType::Binary, N);
    ShadingPipeline pipeline_fine(fine, ShadingAlgorithmType::Binary, N);
    HybridForceTorqueCalculator hybrid_coarse(coarse, pipeline_coarse, newton);
    HybridForceTorqueCalculator hybrid_fine(fine, pipeline_fine, newton);
    const Errors eh = relative_errors(evaluate(hybrid_fine, v), evaluate(hybrid_coarse, v), radius);
    std::printf("[          ] hybrid, coarse vs fine back plate: force %.2e torque %.2e (for comparison)\n", eh.force, eh.torque);
}

// (e) The error against the analytic result shrinks as num_pixel grows.
TEST(PixelAccuracyTest, ErrorShrinksWithResolution) {
    Newton newton;
    InMemoryGeometry plates;
    add_two_plates(plates);
    const double radius = plates.get_bounding_sphere_radius();
    constexpr double TAN_ALPHA = 0.25;
    constexpr double EDGES = 16.0;
    const PlateReference reference = two_plates_reference(TAN_ALPHA);
    const double projected_area = reference.wetted_area / std::sqrt(1.0 + TAN_ALPHA * TAN_ALPHA);

    std::vector<double> errors;
    const std::array<unsigned int, 6> resolutions{64u, 128u, 256u, 512u, 1024u, 2048u};
    for (unsigned int num_pixel : resolutions) {
        PixelForceTorqueCalculator pixel(plates, newton, num_pixel);
        const Errors e = relative_errors(evaluate(pixel, plate_flow(TAN_ALPHA)), reference.loads, radius);
        const double error = std::max(e.force, e.torque);
        const double bound = edge_bound(EDGES, radius, num_pixel, projected_area);
        std::printf("[          ] N=%4u error %.2e (bound %.2e)\n", num_pixel, error, bound);
        EXPECT_LT(error, bound) << "N=" << num_pixel;
        errors.push_back(error);
    }
    // 32x the resolution; the bound falls 32x, require at least 8x.
    EXPECT_LT(errors.back(), errors.front() / 8.0);
}
