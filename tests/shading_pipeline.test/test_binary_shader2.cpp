#include "pch.h"
#include "Binary_Shader2.h"
#include "tetraeder.h"

TEST(BinaryShader2Test, ShadeSatellite) {
	BinaryShader2 shader(vertices, sizeof(vertices) / sizeof(float), triangleIDs, sizeof(triangleIDs) / sizeof(unsigned int));
	float isTriangleVisible[sizeof(triangleIDs) / sizeof(unsigned int)] = { 0 };
	glm::vec3 windDir(1.0f, 0.0f, 0.0f);
	float bounding_sphere_radius = 1.0f;
	shader.shade_satellite(isTriangleVisible, sizeof(isTriangleVisible) / sizeof(float), windDir, bounding_sphere_radius);
	// Since the tetrahedron is centered at the origin and we are looking along the positive x-axis,
	// we expect all 4 faces to be visible.
	EXPECT_NEAR(isTriangleVisible[0], 0.0f, 1e-5);
	EXPECT_NEAR(isTriangleVisible[1], 0.0f, 1e-5);
	EXPECT_NEAR(isTriangleVisible[2], 0.0f, 1e-5);
	EXPECT_NEAR(isTriangleVisible[3], 1.0f, 1e-5);

	windDir = glm::vec3(0.0f, 0.0f, 1.0f);
	std::fill_n(isTriangleVisible, sizeof(isTriangleVisible) / sizeof(float), 0.0f);
	bounding_sphere_radius = 1.0f;
	shader.shade_satellite(isTriangleVisible, sizeof(isTriangleVisible) / sizeof(float), windDir, bounding_sphere_radius);
	// Since the tetrahedron is centered at the origin and we are looking along the positive x-axis,
	// we expect all 4 faces to be visible.
	EXPECT_NEAR(isTriangleVisible[0], 0.0f, 1e-5);
	EXPECT_NEAR(isTriangleVisible[1], 1.0f, 1e-5);
	EXPECT_NEAR(isTriangleVisible[2], 1.0f, 1e-5);
	EXPECT_NEAR(isTriangleVisible[3], 0.0f, 1e-5);


}