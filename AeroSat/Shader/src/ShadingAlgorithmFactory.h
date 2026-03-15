#pragma once
#include <memory>
#include "src/IShading_Algorithm.h"

enum class ShadingAlgorithmType {
    Binary
};

std::unique_ptr<IShadingAlgorithm> create_shading_algorithm(ShadingAlgorithmType type, unsigned int num_pixel);