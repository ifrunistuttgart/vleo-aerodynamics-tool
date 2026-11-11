#pragma once
#include "Igsi_model.h"

namespace vat::gsi_models {

/**
 * Dummy GSI model that measures the projected area instead of a physical load.
 *
 * For every windward element it returns the force (A * cos(delta), 0, 0), where A is the element area and
 * delta the angle between the surface normal and the flow. Summed by a load calculator, the first component
 * of the total force is therefore the area of the geometry projected normal to the flow (with shadowing
 * applied if the calculator accounts for it). Leeward elements contribute nothing. The other force
 * components are zero; the torque is only cross(centroid, force) and has no physical meaning.
 */
class ProjectedArea : public IGSIModel {
public:
    ProjectedArea() = default;
    ~ProjectedArea() override = default;

    int calc_aero_force_and_torque(float area__m2, const glm::vec3& normal, const glm::vec3& centroid__m, const glm::vec3& v_rel__m_per_s, float surf_temp__K, AeroConditions& aero, glm::vec3& aero_force__N, glm::vec3& aero_torque__Nm) override;
    [[nodiscard]] glm::vec3 force_per_area(const glm::vec3& normal, const glm::vec3& v_rel__m_per_s, float surf_temp__K, const AeroConditions& aero) const override;
    [[nodiscard]] std::string glsl_force_per_projected_area() const override;
    [[nodiscard]] std::vector<GlslUniform> glsl_uniforms(const AeroConditions& aero) const override;
    void set_gsi_parameter(std::string name, float value) override;
    [[nodiscard]] float get_gsi_parameter(std::string name) const override;
};

} // namespace vat::gsi_models
