#pragma once
#include <memory>
#include "Ishading_algorithm.h"

namespace vat::shading {

/**
 * @brief Which shading algorithm decides triangle visibility.
 *
 * Binary: a triangle is visible if any of its fragments survives the depth test (BinaryShader).
 * CoP: a triangle is visible if its centroid survives the depth test (CoPShader).
 */
enum class ShadingAlgorithmType { Binary, CoP };

/**
 * @brief Creates a shading algorithm of the given type.
 *
 * @param type Algorithm to create.
 * @param num_pixel Edge length of the square render target.
 * @param num_msaa_samples Samples per pixel. 1 means no anti-aliasing; > 1 enables MSAA and
 *                    is only supported by Binary.
 * @return The algorithm; set_vertices() has not been called yet.
 * @throws std::invalid_argument if num_msaa_samples is 0, or > 1 for an algorithm other than Binary.
 * @throws std::runtime_error for an unknown type.
 */
std::unique_ptr<IShadingAlgorithm> create_shading_algorithm(ShadingAlgorithmType type, unsigned int num_pixel,
                                                            unsigned int num_msaa_samples = 1);

} // namespace vat::shading
