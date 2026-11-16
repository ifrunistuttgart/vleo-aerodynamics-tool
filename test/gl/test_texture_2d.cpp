#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <numeric>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "compute_shader.h"
#include "frame_buffer.h"
#include "gl_helpers.h"
#include "glfw_opengl_context.h"
#include "memory_barrier.h"
#include "scoped_context.h"
#include "shader_storage_buffer.h"
#include "texture_2d.h"

using namespace vat::gl;

namespace {

// One hidden context per test; the textures must be destroyed before it.
class Texture2DTest : public ::testing::Test {
protected:
    Texture2DTest() : context(64, 64, "Texture2D test", false), current(context) {}

    int max_color_samples() const {
        GLint samples = 1;
        glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &samples);
        return std::max(1, static_cast<int>(samples));
    }

    GlfwOpenGLContext context;
    ScopedCurrentContext current;
};

} // namespace

TEST_F(Texture2DTest, SingleSampleUsesTexture2DTarget) {
    Texture2D texture(8, 4, GL_R32F, GL_RED, GL_FLOAT);
    ASSERT_TRUE(texture.is_valid());
    EXPECT_EQ(texture.get_target(), static_cast<unsigned int>(GL_TEXTURE_2D));
    EXPECT_FALSE(texture.is_multisample());
    EXPECT_EQ(texture.get_num_samples(), 1);
    EXPECT_EQ(texture.get_width(), 8);
    EXPECT_EQ(texture.get_height(), 4);
    EXPECT_EQ(texture.get_internal_format(), static_cast<unsigned int>(GL_R32F));
}

TEST_F(Texture2DTest, ReadBackReturnsUploadedData) {
    constexpr int width = 5;
    constexpr int height = 3;
    Texture2D texture(width, height, GL_R32F, GL_RED, GL_FLOAT);
    ASSERT_TRUE(texture.is_valid());

    std::vector<float> data(width * height);
    std::iota(data.begin(), data.end(), 1.0f);
    texture.bind();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    GLCall(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_FLOAT, data.data()));
    texture.unbind();

    EXPECT_EQ(texture.read_back_float(), data);
}

TEST_F(Texture2DTest, ClearZeroesTheTexture) {
    Texture2D texture(4, 4, GL_R32F, GL_RED, GL_FLOAT);
    const std::vector<float> ones(16, 1.0f);
    texture.bind();
    GLCall(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RED, GL_FLOAT, ones.data()));
    texture.unbind();

    texture.clear();
    EXPECT_EQ(texture.read_back_float(), std::vector<float>(16, 0.0f));
}

TEST_F(Texture2DTest, FrameBufferClearClearsAttachedTextures) {
    auto texture = std::make_shared<Texture2D>(4, 4, GL_R32F, GL_RED, GL_FLOAT);
    const std::vector<float> ones(16, 1.0f);
    texture->bind();
    GLCall(glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RED, GL_FLOAT, ones.data()));
    texture->unbind();

    const std::array<std::shared_ptr<Texture2D>, 1> attachments{texture};
    FrameBuffer frame_buffer(4, 4);
    frame_buffer.attach_color_textures(attachments);
    frame_buffer.Clear();
    frame_buffer.UnBind();
    EXPECT_EQ(texture->read_back_float(), std::vector<float>(16, 0.0f));
}

TEST_F(Texture2DTest, MultisampleUsesMultisampleTargetAndCannotBeReadBack) {
    const int samples = std::min(4, max_color_samples());
    if (samples < 2) {
        GTEST_SKIP() << "driver has no multisample textures";
    }
    Texture2D texture(16, 16, GL_RGBA32F, GL_RGBA, GL_FLOAT, samples);
    ASSERT_TRUE(texture.is_valid());
    EXPECT_EQ(texture.get_target(), static_cast<unsigned int>(GL_TEXTURE_2D_MULTISAMPLE));
    EXPECT_TRUE(texture.is_multisample());
    EXPECT_EQ(texture.get_num_samples(), samples);
    EXPECT_THROW((void)texture.read_back_float(), std::logic_error);
}

TEST_F(Texture2DTest, ImpossibleAllocationIsReportedNotAsserted) {
    Texture2D texture(16, 16, GL_RGBA32F, GL_RGBA, GL_FLOAT, 4096);
    EXPECT_FALSE(texture.is_valid());
    EXPECT_EQ(texture.get_texture_id(), 0u);
}

TEST_F(Texture2DTest, MoveTransfersOwnership) {
    Texture2D source(4, 4, GL_R32F, GL_RED, GL_FLOAT);
    const unsigned int id = source.get_texture_id();
    ASSERT_NE(id, 0u);

    Texture2D moved(std::move(source));
    EXPECT_EQ(moved.get_texture_id(), id);
    EXPECT_FALSE(source.is_valid()); // NOLINT(bugprone-use-after-move): the state is specified

    Texture2D assigned(2, 2, GL_R32F, GL_RED, GL_FLOAT);
    assigned = std::move(moved);
    EXPECT_EQ(assigned.get_texture_id(), id);
    EXPECT_FALSE(moved.is_valid()); // NOLINT(bugprone-use-after-move)
    EXPECT_EQ(assigned.get_width(), 4);
}

TEST_F(Texture2DTest, FrameBufferTakesSamplesFromTextures) {
    const int samples = std::min(4, max_color_samples());
    auto a = std::make_shared<Texture2D>(16, 16, GL_RGBA32F, GL_RGBA, GL_FLOAT, samples);
    auto b = std::make_shared<Texture2D>(16, 16, GL_RGBA32F, GL_RGBA, GL_FLOAT, samples);
    ASSERT_TRUE(a->is_valid());
    ASSERT_TRUE(b->is_valid());

    const std::array<std::shared_ptr<Texture2D>, 2> attachments{a, b};
    FrameBuffer frame_buffer(16, 16, samples);
    frame_buffer.attach_color_textures(attachments);
    frame_buffer.UnBind();
    EXPECT_TRUE(frame_buffer.is_complete());
}

TEST_F(Texture2DTest, FrameBufferRejectsMismatchedTextures) {
    auto a = std::make_shared<Texture2D>(16, 16, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    auto b = std::make_shared<Texture2D>(8, 8, GL_RGBA32F, GL_RGBA, GL_FLOAT);
    const std::array<std::shared_ptr<Texture2D>, 2> attachments{a, b};
    FrameBuffer frame_buffer(16, 16);
    EXPECT_THROW(frame_buffer.attach_color_textures(attachments), std::invalid_argument);
    EXPECT_TRUE(frame_buffer.is_complete()); // unchanged: still the depth-only framebuffer
}

TEST_F(Texture2DTest, FrameBufferRejectsNullTexture) {
    const std::array<std::shared_ptr<Texture2D>, 1> attachments{nullptr};
    FrameBuffer frame_buffer(16, 16);
    EXPECT_THROW(frame_buffer.attach_color_textures(attachments), std::invalid_argument);
}

TEST_F(Texture2DTest, FrameBufferKeepsAttachedTexturesAlive) {
    FrameBuffer frame_buffer(4, 4);
    {
        auto texture = std::make_shared<Texture2D>(4, 4, GL_R32F, GL_RED, GL_FLOAT);
        const std::array<std::shared_ptr<Texture2D>, 1> attachments{texture};
        frame_buffer.attach_color_textures(attachments);
    } // the caller's reference is gone, the framebuffer's is not
    frame_buffer.Clear();
    frame_buffer.UnBind();
    EXPECT_TRUE(frame_buffer.is_complete());
}

// --- ShaderStorageBuffer, ComputeShader::dispatch and memory_barrier ---

TEST_F(Texture2DTest, ShaderStorageBufferRoundTripsData) {
    const std::vector<float> data{1.0f, 2.0f, 3.0f, 4.0f};
    const ShaderStorageBuffer buffer(std::as_bytes(std::span<const float>(data)), BufferUsage::DynamicRead);
    EXPECT_EQ(buffer.get_size_bytes(), data.size() * sizeof(float));

    std::vector<float> read(data.size());
    buffer.read_back(std::span<float>(read));
    EXPECT_EQ(read, data);

    std::vector<float> too_big(data.size() + 1);
    EXPECT_THROW(buffer.read_back(std::span<float>(too_big)), std::out_of_range);
}

TEST_F(Texture2DTest, ShaderStorageBufferClearZeroesIt) {
    const std::vector<std::uint32_t> data{7u, 8u, 9u};
    const ShaderStorageBuffer buffer(std::as_bytes(std::span<const std::uint32_t>(data)), BufferUsage::DynamicRead);
    buffer.clear();

    std::vector<std::uint32_t> read(data.size(), 1u);
    buffer.read_back(std::span<std::uint32_t>(read));
    EXPECT_EQ(read, std::vector<std::uint32_t>(data.size(), 0u));
}

TEST_F(Texture2DTest, ShaderStorageBufferMoveTransfersOwnership) {
    ShaderStorageBuffer source(16);
    const unsigned int id = source.get_buffer_id();
    ASSERT_NE(id, 0u);

    ShaderStorageBuffer moved(std::move(source));
    EXPECT_EQ(moved.get_buffer_id(), id);
    EXPECT_EQ(source.get_buffer_id(), 0u); // NOLINT(bugprone-use-after-move): the state is specified
}

TEST_F(Texture2DTest, ComputeShaderDispatchWritesToBuffer) {
    ComputeShader shader(R"GLSL(
#version 430
layout(local_size_x = 4) in;
layout(std430, binding = 0) buffer Out { uint values[]; };
void main() { values[gl_GlobalInvocationID.x] = gl_GlobalInvocationID.x + 1u; }
)GLSL", true);
    const ShaderStorageBuffer buffer(8 * sizeof(std::uint32_t));
    buffer.bind_base(0);
    shader.Bind();
    shader.dispatch(2);
    memory_barrier(Barrier::ShaderStorage | Barrier::BufferUpdate);
    shader.Unbind();

    std::vector<std::uint32_t> read(8);
    buffer.read_back(std::span<std::uint32_t>(read));
    EXPECT_EQ(read, (std::vector<std::uint32_t>{1, 2, 3, 4, 5, 6, 7, 8}));
}
