#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include "in_memory_geometry.h"
#include "pixel_projected_area.h"

using vat::projected_area::PixelProjectedArea;

// A unit square seen face-on shows its full area.
TEST(PixelProjectedAreaTest, FaceOnPlateGivesItsArea) {
    InMemoryGeometry plate;
    // Counter-clockwise seen from +x, so the normal is +x.
    plate.add_quad(glm::vec3(0.0f, -0.5f, -0.5f), glm::vec3(0.0f, 0.5f, -0.5f), glm::vec3(0.0f, 0.5f, 0.5f),
        glm::vec3(0.0f, -0.5f, 0.5f));

    constexpr unsigned int num_pixel = 512;
    PixelProjectedArea projected_area(plate, num_pixel);

    // The render target spans the bounding sphere; allow a few pixel rows of error along the edges.
    const float radius = plate.get_bounding_sphere_radius();
    const float pixel_size = 2.0f * radius / num_pixel;
    EXPECT_NEAR(projected_area.compute_projected_area(glm::vec3(1.0f, 0.0f, 0.0f)), 1.0f, 4.0f * 2.0f * pixel_size);
}
