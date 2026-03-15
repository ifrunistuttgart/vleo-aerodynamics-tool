#include "ShadingPipeline.h"


ShadingPipeline::ShadingPipeline(ISatellite& satellite, IShadingAlgorithm& algorithm) 
: m_satellite(satellite), m_algorithm(algorithm) 
{

}

int ShadingPipeline::set_satellite(ISatellite& satellite) {
	return 0;
}

int ShadingPipeline::shade(const Eigen::Vector3f& v_rel_hat) {
	return 0;
}

