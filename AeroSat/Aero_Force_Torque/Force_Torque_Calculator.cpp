#include "Force_Torque_Calculator.h"

/**
 * @param satellite
 */
ForceTorqueCalculator::ForceTorqueCalculator(ISatellite* satellite) {

}

ForceTorqueCalculator::~ForceTorqueCalculator() {

}
/**
 * @param v_rel__m_per_s
 * @param aero
 */
int ForceTorqueCalculator::calc_aero_torque_force(const Eigen::Vector3f& v_rel__m_per_s, AeroConditions aero) {
	return 0;
}

/**
 * @param satellite
 */
int ForceTorqueCalculator::change_satellite(ISatellite* satellite) {
	return 0;
}

