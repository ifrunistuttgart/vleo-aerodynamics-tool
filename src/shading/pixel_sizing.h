#pragma once
#include "Igeometry_shading_data.h"
#include "shading_algorithm_factory.h"

namespace vat::shading {

/*
 * Choosing num_pixel from the mesh.
 *
 * The orthographic frustum spans the bounding sphere, 2R across, so one pixel is
 * 2R / num_pixel wide. A triangle is lit or shadowed as a whole, and what decides how
 * well the raster resolves it is its narrowest width w (smallest altitude), not its area.
 * Asking for k pixels across w gives num_pixel = 2R k / w.
 *
 * Below about 2.1 pixels both algorithms lose triangles outright: Binary sees a triangle
 * only if it covers a pixel centre, and CoP draws each centroid into the ID image as a
 * point, so two centroids in one pixel leave only the later one visible. (Incircle, and
 * centroid spacing, of 2w/3 against a pixel diagonal gives w >= 2.12 pixels.)
 *
 * Above that the two part ways. Measured on the remeshed shuttlecock against a reference
 * of 307k triangles at num_pixel 8192, worst force error over eight flow directions:
 *
 * - CoP keeps improving up to 6-7 pixels across the 5th-percentile width and is flat after,
 *   at the mesh's own discretisation error: 21k triangles 3.0 % at 3 px, 1.1 % at 4.6 px,
 *   0.13 % at 6.1 px; 82k triangles 0.44 % at 5.2 px, 0.11 % at 7.0 px.
 * - Binary does not improve with pixels at all. "Visible if any pixel shows it" counts every
 *   partly shadowed triangle as fully lit, so its error is set by triangle size -- 5 %, 2.5 %
 *   and 1.3 % at 6k, 21k and 82k triangles -- and more pixels only grow it slightly.
 *
 * Hence 7 pixels for CoP and 3 for Binary, the no-loss bound plus margin.
 */

/** Pixels across a triangle's narrowest width that suggest_num_pixel() aims for. */
float pixels_per_triangle_width(ShadingAlgorithmType type);

/** Smallest num_pixel suggest_num_pixel() returns, so tiny meshes are still rasterised finely. */
inline constexpr unsigned int MIN_AUTO_NUM_PIXEL = 256;
/**
 * Largest num_pixel suggest_num_pixel() returns: the R32UI ID image alone is then 256 MB.
 * A mesh that asks for more has triangles too thin for any sensible raster and should be
 * remeshed instead.
 */
inline constexpr unsigned int MAX_AUTO_NUM_PIXEL = 8192;

/**
 * num_pixel = ceil(2 R k / w), with k from pixels_per_triangle_width(). Not clamped.
 *
 * @throws std::invalid_argument if either length is not positive and finite.
 */
unsigned int num_pixel_for_triangle_width(float bounding_sphere_radius__m, float triangle_width__m,
    ShadingAlgorithmType type);

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
unsigned int suggest_num_pixel(IGeometryShadingData& geometry, ShadingAlgorithmType type);

} // namespace vat::shading
