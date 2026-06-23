#pragma once
#include <memory>
#include "Ishading_algorithm.h"

enum class ShadingAlgorithmType {
    Binary,
    CoP
};

std::unique_ptr<IShadingAlgorithm> create_shading_algorithm(ShadingAlgorithmType type, unsigned int num_pixel);