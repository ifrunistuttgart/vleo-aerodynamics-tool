#include "shading_algorithm_factory.h"
#include "binary_shader/binary_shader.h"
#include "cop_shader/cop_shader.h"
#include <stdexcept>

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

std::unique_ptr<IShadingAlgorithm> create_shading_algorithm(
    ShadingAlgorithmType type,
    unsigned int num_pixel) {

    switch (type) {
    case ShadingAlgorithmType::Binary:
        return std::make_unique<BinaryShader>(num_pixel);
    case ShadingAlgorithmType::CoP:
        return std::make_unique<CoPShader>(num_pixel);
    default:
        SPDLOG_ERROR("Unknown ShadingAlgorithmType: {}", static_cast<int>(type));
        throw std::runtime_error("Unknown ShadingAlgorithmType");
    }
}
