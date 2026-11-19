#pragma once
#include <memory>
#include "Ishading_algorithm.h"

namespace vat::shading {

enum class ShadingAlgorithmType {
    Binary,
    CoP
};

/**
 * @param num_msaa_samples Samples per pixel. 1 means no anti-aliasing; > 1 enables MSAA and
 *                    is only supported by Binary.
 * @throws std::invalid_argument if num_msaa_samples is 0, or > 1 for an algorithm other than Binary.
 */
std::unique_ptr<IShadingAlgorithm> create_shading_algorithm(ShadingAlgorithmType type, unsigned int num_pixel,
    unsigned int num_msaa_samples = 1);

} // namespace vat::shading
