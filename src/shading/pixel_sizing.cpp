#include "pixel_sizing.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include "mesh_quality.h"

namespace vat::shading {

float pixels_per_triangle_width(ShadingAlgorithmType type) {
    switch (type) {
    case ShadingAlgorithmType::Binary:
        return 3.0f;
    case ShadingAlgorithmType::CoP:
        return 7.0f;
    }
    throw std::invalid_argument("unknown ShadingAlgorithmType");
}

unsigned int num_pixel_for_triangle_width(float bounding_sphere_radius__m, float triangle_width__m,
    ShadingAlgorithmType type) {
    if (!(std::isfinite(bounding_sphere_radius__m) && bounding_sphere_radius__m > 0.0f)) {
        throw std::invalid_argument("bounding sphere radius must be positive and finite");
    }
    if (!(std::isfinite(triangle_width__m) && triangle_width__m > 0.0f)) {
        throw std::invalid_argument("triangle width must be positive and finite");
    }
    const double num_pixel = std::ceil(2.0 * bounding_sphere_radius__m * pixels_per_triangle_width(type)
        / triangle_width__m);
    return static_cast<unsigned int>(std::min(num_pixel, 4294967295.0));
}

unsigned int suggest_num_pixel(IGeometryShadingData& geometry, ShadingAlgorithmType type) {
    const geometry::MeshQuality quality = geometry::compute_mesh_quality(geometry);
    const float width__m = quality.min_altitude__m.p05;

    // A mesh whose thinnest 5 % have zero width cannot be sized from them at all.
    const unsigned int wanted = width__m > 0.0f
        ? num_pixel_for_triangle_width(quality.bounding_sphere_radius__m, width__m, type)
        : MAX_AUTO_NUM_PIXEL + 1;
    const unsigned int num_pixel = std::clamp(wanted, MIN_AUTO_NUM_PIXEL, MAX_AUTO_NUM_PIXEL);

    if (quality.aspect_ratio.median > 3.0f) {
        SPDLOG_WARN("The mesh is made of slivers (median aspect ratio {:.3g}; 1 is equilateral). "
            "num_pixel = {} has to be sized for their narrow side; remeshing gives the same "
            "accuracy with far fewer pixels.", quality.aspect_ratio.median, num_pixel);
    }
    if (wanted > MAX_AUTO_NUM_PIXEL) {
        SPDLOG_WARN("The mesh's thinnest triangles would need num_pixel = {}; capped at {}. The "
            "thinnest ones can now fall between pixels -- remesh the geometry.", wanted, MAX_AUTO_NUM_PIXEL);
    }
    SPDLOG_INFO("num_pixel = {} (R = {:.4g} m, 5th-percentile triangle width = {:.4g} m)",
        num_pixel, quality.bounding_sphere_radius__m, width__m);
    return num_pixel;
}

} // namespace vat::shading
