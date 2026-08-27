#pragma once
#include <span>
#include <vector>
#include <glm/glm.hpp>
#include "Igeometry_shading_data.h"    

namespace vat {

/**
 * Interface for the shading pipeline.
 *
 * The shading pipeline is responsible for determining the visibility (shading) of
 * the triangles of a geometry from the perspective of the incoming gas flow.
 * This is used to determine which triangles are exposed to the flow and thus
 * contribute to the aerodynamic loads.
 */
class IShadingPipeline {
    public:
    /**
     * Calculates the visibility/shading for all triangles in the geometry.
     *
     * @param v_rel_hat The normalized relative velocity vector of the incoming flow
     *                  in the satellite's body frame.
     * @return A vector containing the visibility factor for each triangle.
     *         Typically, 1.0 means fully exposed and 0.0 means fully shaded/occluded.
     */
    virtual std::vector<float> shade(const glm::vec3& v_rel_hat) = 0;

	virtual ~IShadingPipeline() = default;
};

} // namespace vat
