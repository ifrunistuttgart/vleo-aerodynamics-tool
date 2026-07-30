#include <gtest/gtest.h>
#define FMT_UNICODE 0
#include <spdlog/spdlog.h>
#include <span>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "cop_shader/cop_shader.h"
#include "geometries/tetraeder_vector.h"


class CoPShaderTest : public ::testing::Test {
protected:
    GLFWwindow* m_window = nullptr;

    void SetUp() override {
        // Initialize GLFW
        if (!glfwInit())
        {
            SPDLOG_ERROR("Failed to initialize GLFW");
            return;
        }

        // Configure GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Fenster unsichtbar machen

        // Create window
        m_window = glfwCreateWindow(800, 800, "Triangle Renderer", nullptr, nullptr);
        if (m_window == nullptr)
        {
			SPDLOG_ERROR("Failed to create GLFW window");
			return;
        }

        glfwMakeContextCurrent(m_window);

        // Initialize GLEW
        if (glewInit() != GLEW_OK)
        {
			SPDLOG_ERROR("Failed to initialize GLEW");
            glfwTerminate();
            return;
        }

		const auto* gl_version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        SPDLOG_INFO("OpenGL context initialized successfully with version: {}", gl_version != nullptr ? gl_version : "unknown");

    }

    void TearDown() override {
        if (m_window != nullptr) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }

        glfwTerminate();
    }
};

TEST_F(CoPShaderTest, ShadeTetrahedron) {
    CoPShader shader(800);

    shader.set_vertices(
        std::span<const float>(vertices),
        std::span<const std::uint32_t>(triangleIDs));

    std::vector<unsigned int> numTrianglesPerMesh{ numFaces };
    std::vector<glm::mat4> modelMatrices{ glm::mat4(1.0f) };
    glm::vec3 windDir(1.0f, 0.0f, 0.0f);
    float bounding_sphere_radius = 1.0f;
    std::vector<float> isTriangleVisible = shader.shade_satellite(
        windDir,
        bounding_sphere_radius,
        std::span<const unsigned int>(numTrianglesPerMesh),
        std::span<const glm::mat4>(modelMatrices));

    EXPECT_NEAR(isTriangleVisible[0], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[1], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[2], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[3], 1.0f, 1e-5);

    windDir = glm::vec3(0.0f, 0.0f, 1.0f);
    std::fill(isTriangleVisible.begin(), isTriangleVisible.end(), 0.0f);
    bounding_sphere_radius = 1.0f;
    isTriangleVisible = shader.shade_satellite(
        windDir,
        bounding_sphere_radius,
        std::span<const unsigned int>(numTrianglesPerMesh),
        std::span<const glm::mat4>(modelMatrices));

    EXPECT_NEAR(isTriangleVisible[0], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[1], 1.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[2], 1.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[3], 0.0f, 1e-5);
}