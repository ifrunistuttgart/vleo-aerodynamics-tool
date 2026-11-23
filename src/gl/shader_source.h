#pragma once
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vat::gl {

/**
 * GLSL source of one shader stage, written as a single template with placeholders.
 *
 * A placeholder is `${name}` (name: letters, digits, underscores); GLSL itself never uses `$`.
 * Every placeholder must be given a value with set() before glsl() returns the code, so a
 * forgotten one is an error here and not a confusing compiler message later.
 *
 *     ShaderSource source(R"GLSL(#version 430 core
 *     #define VAT_MSAA_SAMPLES ${num_msaa_samples}
 *     ${gsi_model}
 *     void main() { ... }
 *     )GLSL");
 *     source.set("num_msaa_samples", 4).set("gsi_model", model_glsl);
 *     Shader shader(vertex_source, source); // calls glsl()
 */
class ShaderSource {
public:
    /** @param glsl The complete shader template, #version line included. */
    explicit ShaderSource(std::string glsl);

    /**
     * Gives the placeholder `${name}` its value; setting it again replaces the value.
     * @throws std::invalid_argument if the template has no such placeholder.
     */
    ShaderSource& set(std::string_view name, std::string value);
    /** Same, with the decimal text of value (e.g. for a #define or an array size). */
    ShaderSource& set(std::string_view name, long long value);

    /** Names of all placeholders in the template, in order of first occurrence. */
    [[nodiscard]] const std::vector<std::string>& placeholders() const { return m_placeholders; }

    /** Whether the placeholder has a value. */
    [[nodiscard]] bool is_set(std::string_view name) const;

    /** Placeholders that have no value yet, in order of first occurrence. */
    [[nodiscard]] std::vector<std::string> unset_placeholders() const;

    /**
     * The template with every placeholder replaced by its value.
     * @throws std::logic_error naming every placeholder that has no value.
     */
    [[nodiscard]] std::string glsl() const;

    /** text with "<line>: " in front of every line, as compiler messages count them. */
    [[nodiscard]] static std::string numbered(const std::string& text);

private:
    [[nodiscard]] const std::string* find_value(std::string_view name) const;

    std::string m_template;
    std::vector<std::string> m_placeholders;
    std::vector<std::pair<std::string, std::string>> m_values;
};

} // namespace vat::gl
