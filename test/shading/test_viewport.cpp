#include <gtest/gtest.h>
#include <cstdint>
#include <span>
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "shading_algorithm_factory.h"

using namespace vat::shading;

namespace {

// A row of small triangles in the plane x = 0, facing +x (counter-clockwise seen
// from +x). Each is 0.02 wide: with bounding sphere radius 1 that is a fraction of
// a pixel at 64 pixels across, but several pixels at 800.
struct SmallTriangles {
    std::vector<float> vertices;
    std::vector<std::uint32_t> ids;
    unsigned int count = 0;
};

SmallTriangles make_small_triangles(unsigned int count) {
    SmallTriangles t;
    t.count = count;
    constexpr float size = 0.02f;
    for (unsigned int i = 0; i < count; ++i) {
        const float y = -0.8f + 1.6f * static_cast<float>(i) / static_cast<float>(count - 1);
        const float z = 0.013f * static_cast<float>(i % 3);   // off the pixel grid
        const float corners[3][3] = { {0.0f, y, z}, {0.0f, y + size, z}, {0.0f, y, z + size} };
        for (const auto& c : corners) {
            t.vertices.insert(t.vertices.end(), c, c + 3);
            t.ids.push_back(i + 1);
        }
    }
    return t;
}

} // namespace

// The hidden window here is only 64x64, standing in for a window the OS clamped
// to the screen size. The shaders must still render at their own num_pixel.
class ShadingViewportTest : public ::testing::TestWithParam<ShadingAlgorithmType> {
protected:
    GLFWwindow* m_window = nullptr;

    void SetUp() override {
        ASSERT_TRUE(glfwInit());
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        m_window = glfwCreateWindow(64, 64, "viewport test", nullptr, nullptr);
        ASSERT_NE(m_window, nullptr);
        glfwMakeContextCurrent(m_window);
        ASSERT_EQ(glewInit(), GLEW_OK);
    }

    void TearDown() override {
        if (m_window != nullptr) {
            glfwDestroyWindow(m_window);
        }
        glfwTerminate();
    }
};

TEST_P(ShadingViewportTest, RendersAtNumPixelNotWindowSize) {
    const SmallTriangles triangles = make_small_triangles(20);
    auto shader = create_shading_algorithm(GetParam(), 800);
    ASSERT_EQ(shader->set_vertices(triangles.vertices, triangles.ids), 0);

    const std::vector<unsigned int> num_triangles_per_mesh{ triangles.count };
    const std::vector<glm::mat4> model_matrices{ glm::mat4(1.0f) };
    const std::vector<float> visible = shader->shade_geometry(
        glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, num_triangles_per_mesh, model_matrices);

    ASSERT_EQ(visible.size(), triangles.count);
    for (unsigned int i = 0; i < triangles.count; ++i) {
        EXPECT_NEAR(visible[i], 1.0f, 1e-5f) << "triangle " << i;
    }
}

INSTANTIATE_TEST_SUITE_P(Algorithms, ShadingViewportTest,
    ::testing::Values(ShadingAlgorithmType::Binary, ShadingAlgorithmType::CoP),
    [](const auto& info) { return info.param == ShadingAlgorithmType::Binary ? "Binary" : "CoP"; });
