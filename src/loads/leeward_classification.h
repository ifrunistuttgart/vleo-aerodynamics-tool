#pragma once
#include <cmath>

namespace vat {

/**
 * @brief Tolerance on cos(delta) = dot(n, v_rel) / |v_rel| at or below which a surface counts
 * as leeward.
 *
 * An edge-on surface has no projected area, so no rasterizer sees it, yet
 * rounding in the flow direction (e.g. sin(pi) != 0) can leave cos(delta) a hair above
 * zero and classify it as windward with visibility 0. The pixel load shader receives
 * the resulting threshold as a uniform (see edge_on_threshold), so it is defined only here.
 */
inline constexpr float EDGE_ON_COS_TOLERANCE = 1e-6f;

/**
 * @brief Threshold on dot(n, v_rel) at or below which a surface is leeward.
 *
 * The threshold is EDGE_ON_COS_TOLERANCE * |v_rel|. Every classification, on the CPU and on
 * the GPU, compares dot(n, v_rel) against this one value.
 *
 * @param rel_speed__m_per_s |v_rel| [m/s].
 * @return EDGE_ON_COS_TOLERANCE * rel_speed__m_per_s [m/s]; 0 if the speed is zero, negative or
 *         not finite, i.e. plain dot(n, v_rel) <= 0.
 */
[[nodiscard]] inline float edge_on_threshold(float rel_speed__m_per_s) {
    return (std::isfinite(rel_speed__m_per_s) && rel_speed__m_per_s > 0.0f) ? EDGE_ON_COS_TOLERANCE * rel_speed__m_per_s
                                                                            : 0.0f;
}

/**
 * @brief Whether a surface is leeward: dot(n, v_rel) <= edge_on_threshold(|v_rel|).
 *
 * @param n_dot_v dot(n, v_rel) with the unit surface normal n [m/s].
 * @param rel_speed__m_per_s |v_rel| [m/s].
 * @return True if leeward. False for a NaN dot product, so degenerate triangles are never
 *         classified as leeward.
 */
[[nodiscard]] inline bool is_leeward(float n_dot_v, float rel_speed__m_per_s) {
    return n_dot_v <= edge_on_threshold(rel_speed__m_per_s);
}

} // namespace vat
