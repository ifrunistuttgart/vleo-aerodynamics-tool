#include "mesh_quality.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace vat::geometry {
namespace {

Distribution Summarize(std::vector<float> values) {
    if (values.empty()) {
        return Distribution{NAN, NAN, NAN, NAN, NAN};
    }
    std::sort(values.begin(), values.end());
    auto at = [&](double q) {
        return values[static_cast<std::size_t>(std::lround(q * static_cast<double>(values.size() - 1)))];
    };
    return Distribution{values.front(), at(0.05), at(0.5), at(0.95), values.back()};
}

double Distance(const float* a, const float* b) {
    const double dx = static_cast<double>(a[0]) - b[0];
    const double dy = static_cast<double>(a[1]) - b[1];
    const double dz = static_cast<double>(a[2]) - b[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace

MeshQuality compute_mesh_quality(IGeometryShadingData& geometry) {
    const unsigned int n = geometry.get_num_triangles();
    if (n == 0) {
        throw std::invalid_argument("cannot compute mesh quality of a geometry without triangles");
    }
    const auto vertices = geometry.get_vertices();

    std::vector<float> areas, aspect_ratios, min_altitudes;
    areas.reserve(n);
    aspect_ratios.reserve(n);
    min_altitudes.reserve(n);

    MeshQuality quality{};
    quality.num_triangles = n;
    double total_area = 0.0;

    // Recomputed from the vertices in double rather than taken from get_areas(): a sliver's
    // inradius is a small difference of large terms, and float would drown it.
    for (unsigned int t = 0; t < n; ++t) {
        const float* p0 = &vertices[9 * t];
        const float* p1 = p0 + 3;
        const float* p2 = p0 + 6;
        const double a = Distance(p1, p2);
        const double b = Distance(p2, p0);
        const double c = Distance(p0, p1);
        const double longest = std::max({a, b, c});

        const double e1[3] = {static_cast<double>(p1[0]) - p0[0], static_cast<double>(p1[1]) - p0[1], static_cast<double>(p1[2]) - p0[2]};
        const double e2[3] = {static_cast<double>(p2[0]) - p0[0], static_cast<double>(p2[1]) - p0[1], static_cast<double>(p2[2]) - p0[2]};
        const double cx = e1[1] * e2[2] - e1[2] * e2[1];
        const double cy = e1[2] * e2[0] - e1[0] * e2[2];
        const double cz = e1[0] * e2[1] - e1[1] * e2[0];
        const double area = 0.5 * std::sqrt(cx * cx + cy * cy + cz * cz);

        total_area += area;
        areas.push_back(static_cast<float>(area));
        min_altitudes.push_back(longest > 0.0 ? static_cast<float>(2.0 * area / longest) : 0.0f);

        if (area > 0.0) {
            const double inradius = area / (0.5 * (a + b + c));
            aspect_ratios.push_back(static_cast<float>(longest / (2.0 * std::sqrt(3.0) * inradius)));
        } else {
            ++quality.num_degenerate;
        }
    }

    quality.total_area__m2 = static_cast<float>(total_area);
    quality.mean_area__m2 = static_cast<float>(total_area / n);
    quality.bounding_sphere_radius__m = geometry.get_bounding_sphere_radius();
    quality.area__m2 = Summarize(std::move(areas));
    quality.aspect_ratio = Summarize(std::move(aspect_ratios));
    quality.min_altitude__m = Summarize(std::move(min_altitudes));
    return quality;
}

} // namespace vat::geometry
