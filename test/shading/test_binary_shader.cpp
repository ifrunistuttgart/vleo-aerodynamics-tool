#include <gtest/gtest.h>
#define FMT_UNICODE 0
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <span>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "binary_shader/binary_shader.h"
#include "geometries/tetraeder_vector.h"

using namespace vat;
using namespace vat::shading;

namespace {

struct Plate {
    std::vector<float> vertices;
    std::vector<std::uint32_t> ids;
    unsigned int count = 0;
};

// A square plate of side `side` in the plane x = `x`, facing +x, meshed as a k x k grid
// of cells split into two triangles each. The grid is rotated in-plane by `rot__deg`,
// so that its edges are not aligned with the pixel grid. IDs start at `first_id`.
void append_plate(Plate& plate, float x, float side, unsigned int k, float rot__deg, std::uint32_t first_id) {
    const float c = std::cos(glm::radians(rot__deg));
    const float s = std::sin(glm::radians(rot__deg));
    auto point = [&](unsigned int i, unsigned int j) {
        const float u = side * (static_cast<float>(i) / static_cast<float>(k) - 0.5f);
        const float w = side * (static_cast<float>(j) / static_cast<float>(k) - 0.5f);
        // (y, z) counter-clockwise seen from +x == counter-clockwise in (u, w).
        return glm::vec3(x, c * u - s * w, s * u + c * w);
    };
    std::uint32_t id = first_id;
    for (unsigned int i = 0; i < k; ++i) {
        for (unsigned int j = 0; j < k; ++j) {
            const glm::vec3 a = point(i, j), b = point(i + 1, j), cc = point(i + 1, j + 1), d = point(i, j + 1);
            const glm::vec3 triangles[2][3] = { { a, b, cc }, { a, cc, d } };
            for (const auto& tri : triangles) {
                for (const auto& p : tri) {
                    plate.vertices.insert(plate.vertices.end(), { p.x, p.y, p.z });
                    plate.ids.push_back(id);
                }
                ++id;
                ++plate.count;
            }
        }
    }
}

std::vector<float> shade(IShadingAlgorithm& shader, const Plate& plate, glm::vec3 flow) {
    EXPECT_EQ(shader.set_vertices(plate.vertices, plate.ids), 0);
    const std::vector<unsigned int> num_triangles_per_mesh{ plate.count };
    const std::vector<glm::mat4> model_matrices{ glm::mat4(1.0f) };
    return shader.shade_geometry(glm::normalize(flow), 1.0f, num_triangles_per_mesh, model_matrices);
}

unsigned int count_visible(std::span<const float> visible, size_t begin, size_t end) {
    return static_cast<unsigned int>(std::count_if(visible.begin() + static_cast<std::ptrdiff_t>(begin),
        visible.begin() + static_cast<std::ptrdiff_t>(end), [](float v) { return v > 0.5f; }));
}

} // namespace

class BinaryShaderTest : public ::testing::Test {
protected:
    GLFWwindow* m_window = nullptr;

    void SetUp() override {
        ASSERT_TRUE(glfwInit());
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Fenster unsichtbar machen
        m_window = glfwCreateWindow(800, 800, "Triangle Renderer", nullptr, nullptr);
        ASSERT_NE(m_window, nullptr);
        glfwMakeContextCurrent(m_window);
        ASSERT_EQ(glewInit(), GLEW_OK);

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

    static void expect_tetrahedron(unsigned int num_msaa_samples) {
        SCOPED_TRACE(::testing::Message() << "num_msaa_samples = " << num_msaa_samples);
        BinaryShader shader(800, num_msaa_samples);
        ASSERT_EQ(shader.set_vertices(vertices, triangleIDs), 0);

        const std::vector<unsigned int> num_triangles_per_mesh{ numFaces };
        const std::vector<glm::mat4> model_matrices{ glm::mat4(1.0f) };

        std::vector<float> visible = shader.shade_geometry(glm::vec3(1.0f, 0.0f, 0.0f), 1.0f,
            num_triangles_per_mesh, model_matrices);
        EXPECT_NEAR(visible[0], 0.0f, 1e-5);
        EXPECT_NEAR(visible[1], 0.0f, 1e-5);
        EXPECT_NEAR(visible[2], 0.0f, 1e-5);
        EXPECT_NEAR(visible[3], 1.0f, 1e-5);

        visible = shader.shade_geometry(glm::vec3(0.0f, 0.0f, 1.0f), 1.0f, num_triangles_per_mesh, model_matrices);
        EXPECT_NEAR(visible[0], 0.0f, 1e-5);
        EXPECT_NEAR(visible[1], 1.0f, 1e-5);
        EXPECT_NEAR(visible[2], 1.0f, 1e-5);
        EXPECT_NEAR(visible[3], 0.0f, 1e-5);
    }
};

TEST_F(BinaryShaderTest, ShadeTetrahedron) {
    expect_tetrahedron(1);
}

TEST_F(BinaryShaderTest, ShadeTetrahedronMsaa) {
    expect_tetrahedron(8);
}

// 40 x 40 cells on a unit plate at 800 px across 2 m: each cell is 10 px wide. The flow
// grazes the plate at sin(a) = 0.03, so each triangle projects to about 10 x 0.3 px.
// Without anti-aliasing many of them cover no pixel centre; every one of them is windward
// and unoccluded, so with 8 samples per pixel all must come out visible.
TEST_F(BinaryShaderTest, GrazingPlateAllWindwardVisibleWithMsaa) {
    Plate plate;
    append_plate(plate, 0.0f, 1.0f, 40, 7.0f, 1);
    const float sin_a = 0.03f;
    const glm::vec3 flow(sin_a, std::sqrt(1.0f - sin_a * sin_a), 0.0f);

    BinaryShader msaa(800, 8);
    const std::vector<float> visible_msaa = shade(msaa, plate, flow);
    if (msaa.get_num_msaa_samples() < 4) {
        GTEST_SKIP() << "driver offers only " << msaa.get_num_msaa_samples() << " MSAA samples";
    }
    BinaryShader single(800, 1);
    const std::vector<float> visible_single = shade(single, plate, flow);

    const unsigned int single_hits = count_visible(visible_single, 0, plate.count);
    const unsigned int msaa_hits = count_visible(visible_msaa, 0, plate.count);

    // Sanity check that the case reproduces the problem.
    EXPECT_LT(single_hits, plate.count) << "1 sample already sees every triangle; the case is too easy";
    EXPECT_EQ(msaa_hits, plate.count) << "1 sample: " << single_hits << " of " << plate.count;
}

// A large plate right in front of a smaller one: nothing of the rear plate may leak
// through the GL_LEQUAL marking pass, and the front plate must be fully visible.
TEST_F(BinaryShaderTest, OccludedPlateStaysHidden) {
    for (unsigned int num_msaa_samples : { 1u, 8u }) {
        SCOPED_TRACE(::testing::Message() << "num_msaa_samples = " << num_msaa_samples);
        Plate plates;
        append_plate(plates, 0.1f, 1.0f, 10, 0.0f, 1);
        const unsigned int front_count = plates.count;
        append_plate(plates, -0.1f, 0.6f, 10, 0.0f, front_count + 1);

        BinaryShader shader(800, num_msaa_samples);
        const std::vector<float> visible = shade(shader, plates, glm::vec3(1.0f, 0.0f, 0.0f));

        EXPECT_EQ(count_visible(visible, 0, front_count), front_count);
        EXPECT_EQ(count_visible(visible, front_count, plates.count), 0u);
    }
}
