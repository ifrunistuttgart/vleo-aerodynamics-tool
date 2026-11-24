#include <gtest/gtest.h>
#define FMT_UNICODE 0
#include <spdlog/spdlog.h>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "cop_shader/cop_shader.h"
#include "geometries/tetraeder_vector.h"

using namespace vat;
using namespace vat::shading;

class CoPShaderTest : public ::testing::Test {
protected:
    GLFWwindow* m_window = nullptr;

    void SetUp() override {
        // Initialize GLFW
        if (!glfwInit()) {
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
        if (m_window == nullptr) {
            SPDLOG_ERROR("Failed to create GLFW window");
            return;
        }

        glfwMakeContextCurrent(m_window);

        // Initialize GLEW
        if (glewInit() != GLEW_OK) {
            SPDLOG_ERROR("Failed to initialize GLEW");
            glfwTerminate();
            return;
        }

        const auto* gl_version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        SPDLOG_INFO("OpenGL context initialized successfully with version: {}",
                    gl_version != nullptr ? gl_version : "unknown");
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

    shader.set_vertices(std::span<const float>(vertices), std::span<const std::uint32_t>(triangleIDs));

    std::vector<unsigned int> numTrianglesPerMesh{numFaces};
    std::vector<glm::mat4> modelMatrices{glm::mat4(1.0f)};
    // Both flow directions are tilted slightly off the axes: along an axis, some
    // faces are seen exactly edge-on and their centroid falls on another face's
    // edge, so the result would depend on where the pixel centres happen to lie.
    glm::vec3 windDir = glm::normalize(glm::vec3(1.0f, 0.1f, 0.2f));
    float bounding_sphere_radius = 1.0f;
    std::vector<float> isTriangleVisible =
        shader.shade_geometry(windDir, bounding_sphere_radius, std::span<const unsigned int>(numTrianglesPerMesh),
                              std::span<const glm::mat4>(modelMatrices));

    EXPECT_NEAR(isTriangleVisible[0], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[1], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[2], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[3], 1.0f, 1e-5);

    windDir = glm::normalize(glm::vec3(-0.2f, 0.1f, 1.0f));
    std::fill(isTriangleVisible.begin(), isTriangleVisible.end(), 0.0f);
    bounding_sphere_radius = 1.0f;
    isTriangleVisible =
        shader.shade_geometry(windDir, bounding_sphere_radius, std::span<const unsigned int>(numTrianglesPerMesh),
                              std::span<const glm::mat4>(modelMatrices));

    EXPECT_NEAR(isTriangleVisible[0], 0.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[1], 1.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[2], 1.0f, 1e-5);
    EXPECT_NEAR(isTriangleVisible[3], 0.0f, 1e-5);
}

namespace {

// Two triangles forming a square of half-side `half` in the plane x = `x`, facing +x.
// IDs start at `first_id`.
void append_quad(std::vector<float>& quad_vertices, std::vector<std::uint32_t>& ids, float x, float half,
                 std::uint32_t first_id) {
    const glm::vec3 a(x, -half, -half), b(x, half, -half), c(x, half, half), d(x, -half, half);
    const glm::vec3 triangles[2][3] = {{a, b, c}, {a, c, d}};
    std::uint32_t id = first_id;
    for (const auto& tri : triangles) {
        for (const auto& p : tri) {
            quad_vertices.insert(quad_vertices.end(), {p.x, p.y, p.z});
            ids.push_back(id);
        }
        ++id;
    }
}

} // namespace

// A large plate right in front of a smaller one: no centroid of the rear plate may leak
// through the GL_LEQUAL marking pass, and the front plate must be fully visible.
TEST_F(CoPShaderTest, OccludedPlateStaysHidden) {
    std::vector<float> plate_vertices;
    std::vector<std::uint32_t> plate_ids;
    append_quad(plate_vertices, plate_ids, 0.1f, 0.5f, 1);  // front, IDs 1-2
    append_quad(plate_vertices, plate_ids, -0.1f, 0.3f, 3); // rear, IDs 3-4

    CoPShader shader(800);
    ASSERT_EQ(shader.set_vertices(plate_vertices, plate_ids), 0);
    const std::vector<unsigned int> num_triangles_per_mesh{4};
    const std::vector<glm::mat4> model_matrices{glm::mat4(1.0f)};
    const std::vector<float> visible =
        shader.shade_geometry(glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, num_triangles_per_mesh, model_matrices);

    ASSERT_EQ(visible.size(), 4u);
    EXPECT_NEAR(visible[0], 1.0f, 1e-5);
    EXPECT_NEAR(visible[1], 1.0f, 1e-5);
    EXPECT_NEAR(visible[2], 0.0f, 1e-5);
    EXPECT_NEAR(visible[3], 0.0f, 1e-5);
}

TEST_F(CoPShaderTest, ShadeBeforeSetVerticesThrows) {
    CoPShader shader(800);
    const std::vector<unsigned int> num_triangles_per_mesh{1};
    const std::vector<glm::mat4> model_matrices{glm::mat4(1.0f)};
    EXPECT_THROW(shader.shade_geometry(glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, num_triangles_per_mesh, model_matrices),
                 std::logic_error);
}
