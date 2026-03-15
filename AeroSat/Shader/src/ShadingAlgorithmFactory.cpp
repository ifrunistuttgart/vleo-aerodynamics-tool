#include "src/ShadingAlgorithmFactory.h"
#include "src/BinaryShader/Binary_Shader.h"
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