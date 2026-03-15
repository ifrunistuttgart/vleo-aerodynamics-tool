#include "pch.h"
#include "Binary_Shader.h"
#include "tetraeder.h"
#include <span>
#include <vector>
#include <cstdint>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

class BinaryShaderTest : public ::testing::Test {
protected:
    GLFWwindow* m_window = nullptr;

    void SetUp() override {
        // Initialize GLFW
        if (!glfwInit())
        {
            std::cout << "Failed to initialize GLFW" << std::endl;
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
            std::cout << "Failed to create GLFW window" << std::endl;
        }

        glfwMakeContextCurrent(m_window);

        // Initialize GLEW
        if (glewInit() != GLEW_OK)
        {
            std::cout << "Failed to initialize GLEW" << std::endl;
            glfwTerminate();
        }

        std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    }

    void TearDown() override {
        if (m_window != nullptr) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }

        glfwTerminate();
    }
};

TEST_F(BinaryShaderTest, ShadeTetrahedron) {
    BinaryShader shader(800);

    shader.set_vertices(
        std::span<const float>(vertices, sizeof(vertices) / sizeof(float)),
        std::span<const std::uint32_t>(triangleIDs, sizeof(triangleIDs) / sizeof(unsigned int)));

    std::vector<float> isTriangleVisible(sizeof(triangleIDs) / sizeof(unsigned int), 0.0f);
    glm::vec3 windDir(1.0f, 0.0f, 0.0f);
    float bounding_sphere_radius = 1.0f;
    shader.shade_satellite(std::span<float>(isTriangleVisible), windDir, bounding_sphere_radius);

    EXPECT_NEAR(isTriangleVisible[0], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[1], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[2], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[3], 1.0f, 1e-5);

    windDir = glm::vec3(0.0f, 0.0f, 1.0f);
    std::fill(isTriangleVisible.begin(), isTriangleVisible.end(), 0.0f);
    bounding_sphere_radius = 1.0f;
    shader.shade_satellite(std::span<float>(isTriangleVisible), windDir, bounding_sphere_radius);

    EXPECT_NEAR(isTriangleVisible[0], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[1], 1.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[2], 1.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[3], 0.0f, 1e-5);
}