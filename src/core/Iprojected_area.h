#pragma once
#include <glm/glm.hpp>

namespace vat {

class IProjectedArea {
public:
    virtual ~IProjectedArea() = default;

    /**
     * Computes the projected area of the geometry in the direction of the given vector.
     *
     * @param direction The direction vector in which to compute the projected area.
     * @return The projected area of the geometry in the specified direction.
     */
    virtual float compute_projected_area(const glm::vec3& direction) const = 0;
};

} // namespace vat
