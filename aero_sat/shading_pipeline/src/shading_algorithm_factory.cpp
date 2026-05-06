#include "src/shading_algorithm_factory.h"
#include "src/binary_shader/binary_shader.h"
#include <stdexcept>

std::unique_ptr<IShadingAlgorithm> create_shading_algorithm(
    ShadingAlgorithmType type,
    unsigned int num_pixel) {

    switch (type) {
    case ShadingAlgorithmType::Binary:
        return std::make_unique<BinaryShader>(num_pixel);
    default:
        throw std::runtime_error("Unknown ShadingAlgorithmType");
    }
}