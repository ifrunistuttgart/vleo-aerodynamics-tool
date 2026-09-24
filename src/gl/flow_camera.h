#pragma once
#include <glm/glm.hpp>

namespace vat::gl {

/**
 * View and projection of an orthographic camera that looks along the flow.
 *
 * The camera sits on the bounding sphere at v_rel_hat * R and looks at the origin, so
 * the surfaces the flow hits (dot(normal, v_rel) > 0) face the camera. The projection
 * spans exactly the bounding sphere: [-R, R] across and [0, 2R] deep. One pixel of an
 * N x N render therefore covers (2R / N)^2 of area normal to the flow.
 */
struct FlowCamera {
    glm::mat4 view;
    glm::mat4 projection;
};

FlowCamera make_flow_camera(const glm::vec3& v_rel_hat, float bounding_sphere_radius);

} // namespace vat::gl
