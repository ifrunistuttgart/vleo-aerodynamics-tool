#pragma once
#include <cstddef>
#include <iterator>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>

namespace vat::visualization {

inline constexpr int kWindowWidth = 1200;
inline constexpr int kWindowHeight = 900;

inline const glm::vec3 kBackgroundColor(0.08f, 0.08f, 0.1f);

/*
 * Okabe-Ito qualitative palette, which stays distinguishable under the common forms of
 * colour blindness. The original palette's black is replaced by a light grey, because
 * black is invisible against kBackgroundColor.
 */
inline const glm::vec3 kMeshPalette[] = {
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
 * Colour of mesh number mesh_index.
 *
 * Wraps around for geometries with more meshes than the palette holds; the repeats stay
 * usable because the legend spells out the id.
 */
inline glm::vec3 MeshColor(std::size_t mesh_index) {
    return kMeshPalette[mesh_index % std::size(kMeshPalette)];
}

// Colours the shading view maps triangle visibility between.
inline const glm::vec3 kVisibleTriangleColor(0.0f, 1.0f, 0.0f);
inline const glm::vec3 kHiddenTriangleColor(0.15f, 0.2f, 0.8f);

// The wind arrow in the shading view.
inline const glm::vec3 kWindColor(1.0f, 0.85f, 0.1f);

// Body-frame axis arrows: x, y, z.
inline const glm::vec3 kAxisColors[] = {
    glm::vec3(1.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f),
    glm::vec3(0.0f, 0.5f, 1.0f),
};

} // namespace vat::visualization
