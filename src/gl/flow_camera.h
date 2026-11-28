#pragma once
#include <glm/glm.hpp>

namespace vat::gl {

/**
 * @brief View and projection of an orthographic camera that looks along the flow.
 *
 * The camera sits on the bounding sphere at v_rel_hat * R and looks at the origin, so
 * the surfaces the flow hits (dot(normal, v_rel) > 0) face the camera. The projection
 * spans exactly the bounding sphere: [-R, R] across and [0, 2R] deep. One pixel of an
 * N x N render therefore covers (2R / N)^2 of area normal to the flow.
 */
struct FlowCamera {
    /** @brief Body frame to camera (lookAt from v_rel_hat * R towards the origin). */
    glm::mat4 view;
    /** @brief Orthographic projection spanning the bounding sphere. */
    glm::mat4 projection;
};

/**
 * @brief Builds the FlowCamera for one flow direction.
 *
 * @param v_rel_hat Unit vector of the velocity of the geometry relative to the atmosphere,
 *                  in the body frame.
 * @param bounding_sphere_radius Radius R of the bounding sphere about the body-frame origin [m];
 *                               must be positive.
 * @return The view and projection matrices.
 */
FlowCamera make_flow_camera(const glm::vec3& v_rel_hat, float bounding_sphere_radius);

} // namespace vat::gl
