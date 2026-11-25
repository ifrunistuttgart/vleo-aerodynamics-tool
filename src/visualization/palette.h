#pragma once
#include <cstddef>
#include <iterator>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>

namespace vat::visualization {

/** @brief Initial window width [px]. */
inline constexpr int WINDOW_WIDTH = 1200;
/** @brief Initial window height [px]. */
inline constexpr int WINDOW_HEIGHT = 900;

/** @brief Background colour of every view. */
inline const glm::vec3 BACKGROUND_COLOR(0.08f, 0.08f, 0.1f);

/**
 * @brief Okabe-Ito qualitative palette, which stays distinguishable under the common forms of
 * colour blindness. The original palette's black is replaced by a light grey, because
 * black is invisible against BACKGROUND_COLOR.
 */
inline const glm::vec3 MESH_PALETTE[] = {
    glm::vec3(0.902f, 0.624f, 0.000f), // orange
    glm::vec3(0.337f, 0.706f, 0.914f), // sky blue
    glm::vec3(0.000f, 0.620f, 0.451f), // bluish green
    glm::vec3(0.941f, 0.894f, 0.259f), // yellow
    glm::vec3(0.000f, 0.447f, 0.698f), // blue
    glm::vec3(0.835f, 0.369f, 0.000f), // vermillion
    glm::vec3(0.800f, 0.475f, 0.655f), // reddish purple
    glm::vec3(0.700f, 0.700f, 0.700f), // light grey
};

/**
 * @brief Colour of mesh number mesh_index.
 *
 * Wraps around for geometries with more meshes than the palette holds; the repeats stay
 * usable because the legend spells out the id.
 *
 * @param mesh_index 0-based mesh index.
 * @return The palette colour.
 */
inline glm::vec3 mesh_color(std::size_t mesh_index) { return MESH_PALETTE[mesh_index % std::size(MESH_PALETTE)]; }

/** @brief Shading view colour of a fully visible triangle. */
inline const glm::vec3 VISIBLE_TRIANGLE_COLOR(0.0f, 1.0f, 0.0f);
/** @brief Shading view colour of a hidden triangle. */
inline const glm::vec3 HIDDEN_TRIANGLE_COLOR(0.15f, 0.2f, 0.8f);

/** @brief Colour of the wind arrow in the shading view. */
inline const glm::vec3 WIND_COLOR(1.0f, 0.85f, 0.1f);

/** @brief Colours of the body-frame axis arrows: x, y, z. */
inline const glm::vec3 AXIS_COLORS[] = {
    glm::vec3(1.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3(0.0f, 0.5f, 1.0f),
};

} // namespace vat::visualization
