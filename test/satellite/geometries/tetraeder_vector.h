#pragma once
#include <array>
#include <vector>
#include <cstdint>

// Define tetrahedron vertex positions
static const std::array<float, 3> v0 = { 0.0f, -0.5f, 0.0f };  // Bottom left
static const std::array<float, 3> v1 = { 0.0f, 0.5f, 0.0f };   // Bottom right
static const std::array<float, 3> v2 = { 0.0f, 0.0f, 0.5f };   // Top
static const std::array<float, 3> v3 = { -0.5f, 0.0f, 0.0f };  // Back

// Tetrahedron vertices without index buffer - 4 triangular faces
static const std::vector<float> vertices = {
    // Face 1: bottom (v0, v3, v1)
    v0[0], v0[1], v0[2],
    v3[0], v3[1], v3[2],
    v1[0], v1[1], v1[2],

    // Face 2: right back (v1, v3, v2)
    v1[0], v1[1], v1[2],
    v3[0], v3[1], v3[2],
    v2[0], v2[1], v2[2],

    // Face 3: left back (v2, v3, v0)
    v2[0], v2[1], v2[2],
    v3[0], v3[1], v3[2],
    v0[0], v0[1], v0[2],

    // Face 4: front (v0, v1, v2)
    v0[0], v0[1], v0[2],
    v1[0], v1[1], v1[2],
    v2[0], v2[1], v2[2]
};

static const std::vector<std::uint32_t> triangleIDs = {
    // Face 1 bottom: Red
    1, 1, 1,

    // Face 2: Green
    2, 2, 2,

    // Face 3: Blue
    3, 3, 3,

    // Face 4: Yellow
    4, 4, 4
};

// Number of triangle ID entries (= number of vertices = 3 per face)
static const unsigned int numTriangles = static_cast<unsigned int>(triangleIDs.size());
// Number of actual triangular faces
static const unsigned int numFaces = numTriangles / 3;
