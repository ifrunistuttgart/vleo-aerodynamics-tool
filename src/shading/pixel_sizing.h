#pragma once
#include "Igeometry_shading_data.h"

namespace vat::shading {

/*
 * Choosing num_pixel from the mesh.
 *
 * The orthographic frustum spans the bounding sphere, 2R across, so one pixel is
 * 2R / num_pixel wide. A triangle is lit or shadowed as a whole, and what decides whether
 * the raster resolves it is its narrowest width w (smallest altitude), not its area:
 *
 * - Binary sees a triangle only if it covers a pixel centre.
 * - CoP draws each triangle's centroid into the ID image as a single point, so when two
 *   centroids land in the same pixel the later one overwrites the earlier, and that
 *   triangle reads as hidden.
 *
 * Both are safe once w reaches 2.12 pixels. For a well-shaped triangle the incircle has
 * diameter 2w/3 and neighbouring centroids are about 2w/3 apart, and a disk or a spacing of
 * one pixel diagonal (sqrt(2) pixels) always contains a pixel centre or separates two
 * points into different pixels: 2w/3 >= sqrt(2) gives w >= 2.12. Measured on a flat panel
 * of 80000 triangles, CoP lost 4.4 % of them at 1.5 pixels, 0.15 % at 2.0 and none from
 * 2.125 on. PIXELS_PER_TRIANGLE_WIDTH adds margin for triangles that are not so well shaped.
 *
 * Pixels finer than that add cost but no accuracy: the triangles are then the coarser grid.
 */

/** Pixels across a triangle's narrowest width that suggest_num_pixel() aims for. */
inline constexpr float PIXELS_PER_TRIANGLE_WIDTH = 3.0f;

/** Smallest num_pixel suggest_num_pixel() returns, so tiny meshes are still rasterised finely. */
inline constexpr unsigned int MIN_AUTO_NUM_PIXEL = 256;
/**
 * Largest num_pixel suggest_num_pixel() returns: the R32UI ID image alone is then 256 MB.
 * A mesh that asks for more has triangles too thin for any sensible raster and should be
 * remeshed instead.
 */
inline constexpr unsigned int MAX_AUTO_NUM_PIXEL = 8192;

/**
 * num_pixel = ceil(2 R PIXELS_PER_TRIANGLE_WIDTH / w). Not clamped.
 *
 * @throws std::invalid_argument if either length is not positive and finite.
 */
unsigned int num_pixel_for_triangle_width(float bounding_sphere_radius__m, float triangle_width__m);

/**
 * Suggests num_pixel for a geometry in its current pose.
 *
 * Sized for the 5th-percentile narrowest triangle width rather than the very narrowest, so
 * a handful of stray slivers cannot drive it up, and clamped to [MIN_AUTO_NUM_PIXEL,
 * MAX_AUTO_NUM_PIXEL]. Logs a warning when the mesh is made of slivers (median aspect
 * ratio above 3) or the value had to be clamped -- in both cases remeshing is the fix.
 *
 * @throws std::invalid_argument if the geometry has no triangles.
 */
unsigned int suggest_num_pixel(IGeometryShadingData& geometry);

} // namespace vat::shading
