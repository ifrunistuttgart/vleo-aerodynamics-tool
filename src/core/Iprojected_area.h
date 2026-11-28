#pragma once
#include <glm/glm.hpp>

namespace vat {

/**
 * @brief Interface for computing the area a geometry shows along a direction.
 */
class IProjectedArea {
public:
    virtual ~IProjectedArea() = default;

    /**
     * @brief Computes the projected area of the geometry in the direction of the given vector.
     *
     * @param direction Direction in the body frame, with the same convention as v_rel in the load
     *                  calculators. Need not be normalised.
     * @return The projected area of the geometry in the specified direction [m^2].
     */
    virtual float compute_projected_area(const glm::vec3& direction) const = 0;
};

} // namespace vat
