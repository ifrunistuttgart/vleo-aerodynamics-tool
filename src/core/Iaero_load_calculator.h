#pragma once
#include "core.h"
#include <glm/glm.hpp>

namespace vat {

/**
 * @brief Flow regime assumed when summing the loads.
 *
 * Hypothermal: the thermal speed of the gas is not negligible against the relative speed,
 * so surfaces facing away from the flow (dot(n, v_rel) <= 0) still receive load.
 * Hyperthermal: the relative speed dominates, so surfaces facing away from the flow
 * are ignored and only windward surfaces contribute.
 */
enum class FlowRegime { Hypothermal, Hyperthermal };

/**
 * @brief Interface for calculating aerodynamic loads on a geometry.
 *
 * This interface provides a common method for calculating aerodynamic torque and force
 * based on the geometry, its motion relative to the atmosphere, and environmental
 * conditions.
 */
class IAeroLoadCalculator {
public:
    /**
     * @brief Selects whether leeward-facing triangles contribute (Hypothermal, default) or not (Hyperthermal).
     * @param regime The regime used by all following evaluations.
     */
    virtual void set_flow_regime(FlowRegime regime) = 0;

    /** @brief The regime currently used by calc_aero_torque_force(). */
    [[nodiscard]] virtual FlowRegime get_flow_regime() const = 0;

    /**
     * @brief Calculates the aerodynamic torque and force acting on the geometry.
     *
     * This method evaluates the impact of gas-surface interactions across all relevant
     * triangles of the geometry, considering atmospheric density, temperature, and
     * the relative velocity vector. Force and torque are expressed in the body frame; the
     * torque is taken about the body-frame origin.
     *
     * @param v_rel__m_per_s Velocity of the geometry relative to the atmosphere, in the body frame [m/s].
     * @param surface_temp__K The uniform surface temperature of the geometry [K].
     * @param aero A structure containing atmospheric properties (density, temperature, particle mass).
     * @param torque__Nm Output: aerodynamic torque about the body-frame origin, in the body frame [Nm].
     * @param force__N Output: aerodynamic force in the body frame [N].
     * @return 0 on success; a zero v_rel__m_per_s also returns 0 with zero force and torque. The
     *         implementations return -1 and set both outputs to NaN if v_rel__m_per_s is not finite.
     */
    virtual int calc_aero_torque_force(const glm::vec3& v_rel__m_per_s, float surface_temp__K, AeroConditions& aero,
                                       glm::vec3& torque__Nm, glm::vec3& force__N) = 0;

    virtual ~IAeroLoadCalculator() = default;
};

} // namespace vat
