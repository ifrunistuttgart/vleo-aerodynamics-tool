#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "shader_source.h"

using vat::gl::ShaderSource;

TEST(ShaderSourceTest, ListsPlaceholdersOnceInOrder) {
    ShaderSource source("#version 430\n#define N ${n}\n${body}\nint a[${n}];\n");
    EXPECT_EQ(source.placeholders(), (std::vector<std::string>{"n", "body"}));
    EXPECT_EQ(source.unset_placeholders(), (std::vector<std::string>{"n", "body"}));
}

TEST(ShaderSourceTest, ReplacesEveryOccurrence) {
    ShaderSource source("#define N ${n}\n${body}\nint a[${n}];\n");
    source.set("n", 4).set("body", std::string("void f() {}"));
    EXPECT_TRUE(source.is_set("n"));
    EXPECT_TRUE(source.unset_placeholders().empty());
    EXPECT_EQ(source.glsl(), "#define N 4\nvoid f() {}\nint a[4];\n");
}

TEST(ShaderSourceTest, SettingAgainReplacesTheValue) {
    ShaderSource source("${x}");
    source.set("x", 1).set("x", 2);
    EXPECT_EQ(source.glsl(), "2");
}

TEST(ShaderSourceTest, ValuesAreNotScannedForPlaceholders) {
    ShaderSource source("${x}");
    source.set("x", std::string("${y}"));
    EXPECT_EQ(source.glsl(), "${y}");
}

TEST(ShaderSourceTest, UnsetPlaceholderIsAnErrorNamingIt) {
    ShaderSource source("${a} ${b} ${c}");
    source.set("b", 0);
    EXPECT_EQ(source.unset_placeholders(), (std::vector<std::string>{"a", "c"}));
    try {
        (void)source.glsl();
        FAIL() << "expected std::logic_error";
    } catch (const std::logic_error& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("${a}"), std::string::npos) << message;
        EXPECT_NE(message.find("${c}"), std::string::npos) << message;
        EXPECT_EQ(message.find("${b}"), std::string::npos) << message;
    }
}

TEST(ShaderSourceTest, UnknownPlaceholderIsRejected) {
    ShaderSource source("${num_samples}");
    EXPECT_THROW(source.set("num_sample", 1), std::invalid_argument);
}

TEST(ShaderSourceTest, MalformedPlaceholderIsRejected) {
    EXPECT_THROW(ShaderSource("${}"), std::invalid_argument);
    EXPECT_THROW(ShaderSource("${open"), std::invalid_argument);
    EXPECT_THROW(ShaderSource("${a-b}"), std::invalid_argument);
}

TEST(ShaderSourceTest, TemplateWithoutPlaceholdersIsReturnedAsIs) {
    const std::string text = "#version 430\nvoid main() {}\n";
    EXPECT_EQ(ShaderSource(text).glsl(), text);
}

TEST(ShaderSourceTest, NumberedPrefixesEveryLine) {
    EXPECT_EQ(ShaderSource::numbered("a\nb\n"), "1: a\n2: b\n");
}
