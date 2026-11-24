#pragma once
#include <span>
#include <vector>
#include <glm/glm.hpp>
#include "Igeometry_shading_data.h"

namespace vat {

/**
 * @brief Interface for the shading pipeline.
 *
 * The shading pipeline is responsible for determining the visibility (shading) of
 * geometry triangles from the perspective of the incoming gas flow.
 * This is used to determine which triangles are exposed to the flow and thus
 * contribute to the aerodynamic loads.
 */
class IShadingPipeline {
public:
    /**
     * @brief Calculates the visibility/shading for all triangles in the geometry model.
     *
     * @param v_rel_hat Unit vector of the velocity of the geometry relative to the atmosphere,
     *                  in the body frame.
     * @return The visibility factor of each triangle, in triangle order. Typically, 1.0 means
     *         fully exposed and 0.0 means fully shaded/occluded. The GPU implementations cull
     *         back faces, so leeward triangles get 0.0.
     */
    virtual std::vector<float> shade(const glm::vec3& v_rel_hat) = 0;

    virtual ~IShadingPipeline() = default;
};

} // namespace vat
