#pragma once
#include "Igeometry_shading_data.h"

namespace vat::geometry {

/**
 * @brief Summary of a per-triangle quantity over all triangles.
 *
 * Percentiles use the nearest-rank convention on the sorted values: p05 is the value
 * at index round(0.05 * (n - 1)).
 */
struct Distribution {
    float min;    /**< Smallest value. */
    float p05;    /**< 5th percentile. */
    float median; /**< 50th percentile. */
    float p95;    /**< 95th percentile. */
    float max;    /**< Largest value. */
};

/**
 * @brief Size and shape statistics of a geometry's triangles, in its current pose.
 *
 * These are the numbers that decide how well a mesh suits the shading pass: a triangle
 * is the unit that is lit or shadowed as a whole, and one narrower than a pixel can fall
 * between the pixel centres and be missed entirely.
 */
struct MeshQuality {
    /** @brief Number of triangles in the geometry. */
    unsigned int num_triangles;
    /** @brief Triangles with zero area. They are excluded from aspect_ratio. */
    unsigned int num_degenerate;
    /** @brief Sum of all triangle areas [m^2]. */
    float total_area__m2;
    /** @brief total_area__m2 / num_triangles [m^2]. */
    float mean_area__m2;
    /** @brief IGeometryShadingData::get_bounding_sphere_radius() in the current pose [m]. */
    float bounding_sphere_radius__m;

    /** @brief Distribution of the triangle areas [m^2]. */
    Distribution area__m2;
    /**
     * @brief Longest edge over (2 * sqrt(3) * inradius): 1 for an equilateral triangle and
     * growing without bound as a triangle becomes a sliver.
     */
    Distribution aspect_ratio;
    /**
     * @brief Smallest altitude, 2 * area / longest edge: how wide the triangle is across its
     * narrowest direction. This, not the area, is what must exceed a pixel for the
     * triangle to be rasterised reliably.
     */
    Distribution min_altitude__m;
};

/**
 * @brief Computes MeshQuality for a geometry.
 *
 * @param geometry Any geometry; its current (possibly turned) vertices are used, which
 *                 changes nothing but bounding_sphere_radius__m, since turning a mesh
 *                 does not change its triangles' shape.
 * @return The statistics of all triangles of the geometry.
 * @throws std::invalid_argument if the geometry has no triangles.
 */
MeshQuality compute_mesh_quality(IGeometryShadingData& geometry);

} // namespace vat::geometry
