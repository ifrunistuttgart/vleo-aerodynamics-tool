#include "shading_algorithm_factory.h"
#include "binary_shader/binary_shader.h"
#include "cop_shader/cop_shader.h"
#include <stdexcept>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

namespace vat::shading {

std::unique_ptr<IShadingAlgorithm> create_shading_algorithm(
    ShadingAlgorithmType type,
    unsigned int num_pixel,
    unsigned int num_msaa_samples) {

    if (num_msaa_samples == 0) {
        throw std::invalid_argument("num_msaa_samples must be at least 1 (1 = no anti-aliasing)");
    }

    switch (type) {
    case ShadingAlgorithmType::Binary:
        return std::make_unique<BinaryShader>(num_pixel, num_msaa_samples);
    case ShadingAlgorithmType::CoP:
        if (num_msaa_samples != 1) {
            throw std::invalid_argument("num_msaa_samples > 1 is only supported by the Binary algorithm");
        }
        return std::make_unique<CoPShader>(num_pixel);
    default:
        SPDLOG_ERROR("Unknown ShadingAlgorithmType: {}", static_cast<int>(type));
        throw std::runtime_error("Unknown ShadingAlgorithmType");
    }
}

} // namespace vat::shading
