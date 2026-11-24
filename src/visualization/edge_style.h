#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>

namespace vat::visualization {

/**
 * @file
 * @brief How the triangle-edge overlay is drawn. Kept apart from the palette because these are
 * tuned against each other and against the fade in EdgeOverlay, not chosen for looks.
 */

/**
 * @brief Colour of the triangle edges.
 *
 * A mid grey rather than a dark one, because the edges have to sit on both the bright
 * green of a lit triangle and the dark blue of a shadowed one; anything near the
 * background colour disappears against the latter.
 */
inline const glm::vec3 kEdgeColor(0.42f, 0.42f, 0.45f);

/**
 * @brief Line width of the triangle edges [px].
 *
 * One pixel is the floor, not a preference: a core-profile GL context only guarantees an
 * aliased line width of 1.0, and glLineWidth clamps to the supported range rather than
 * failing. A smaller value here renders no thinner. Lighten kEdgeColor instead to make
 * the overlay recede further.
 */
inline constexpr float kEdgeLineWidth = 1.0f;

/**
 * @brief Lower end of the screen-space fade band, in pixels of typical triangle size.
 *
 * kEdgeFadeMinPx to kEdgeFadeFullPx is the fade band. At one pixel a
 * triangle is entirely covered by the line tracing it, so the overlay says nothing and
 * only greys the model out; by three the triangulation is legible again.
 *
 * These are low on purpose. Measured on shuttlecock_15k.obj, a typical triangle spans
 * just 2.98 px at the framing ResetCamera picks, so a band anchored any higher would
 * erase the edges at the default view rather than only when zoomed out.
 */
inline constexpr float kEdgeFadeMinPx = 1.0f;
/** @brief Upper end of the fade band; from here on the edges have full weight. */
inline constexpr float kEdgeFadeFullPx = 3.0f;

/** @brief Below this weight the edges are switched off rather than drawn imperceptibly. */
inline constexpr float kEdgeInvisibleAlpha = 0.01f;

/**
 * @brief Key that toggles the edge overlay.
 *
 * VTK's own bindings claim most letters -- w/s switch wireframe and surface, e/q exit,
 * r resets the camera -- so the toggle has to be one vtkInteractorStyle leaves alone in
 * OnChar(). 'j' and 't' are only bound by vtkInteractorStyleSwitch, which is not used.
 */
inline constexpr const char* kEdgeToggleKey = "t";

} // namespace vat::visualization
