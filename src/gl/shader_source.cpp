#include "shader_source.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace vat::gl {

namespace {

bool is_name_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

// Calls visit(begin, end, name) for every `${name}` in text, in order; [begin, end) spans the
// whole placeholder.
template <typename Visit>
void for_each_placeholder(const std::string& text, Visit visit) {
    size_t pos = 0;
    while ((pos = text.find("${", pos)) != std::string::npos) {
        size_t name_end = pos + 2;
        while (name_end < text.size() && is_name_char(text[name_end])) {
            ++name_end;
        }
        if (name_end == pos + 2 || name_end >= text.size() || text[name_end] != '}') {
            throw std::invalid_argument("ShaderSource: malformed placeholder at offset " + std::to_string(pos)
                + "; expected ${name} with name made of letters, digits and underscores");
        }
        visit(pos, name_end + 1, std::string_view(text).substr(pos + 2, name_end - pos - 2));
        pos = name_end + 1;
    }
}

} // namespace

ShaderSource::ShaderSource(std::string glsl) : m_template(std::move(glsl)) {
    for_each_placeholder(m_template, [this](size_t, size_t, std::string_view name) {
        if (std::find(m_placeholders.begin(), m_placeholders.end(), name) == m_placeholders.end()) {
            m_placeholders.emplace_back(name);
        }
    });
}

ShaderSource& ShaderSource::set(std::string_view name, std::string value) {
    if (std::find(m_placeholders.begin(), m_placeholders.end(), name) == m_placeholders.end()) {
        throw std::invalid_argument("ShaderSource: the template has no placeholder ${" + std::string(name) + "}");
    }
    for (auto& [existing, existing_value] : m_values) {
        if (existing == name) {
            existing_value = std::move(value);
            return *this;
        }
    }
    m_values.emplace_back(std::string(name), std::move(value));
    return *this;
}

ShaderSource& ShaderSource::set(std::string_view name, long long value) {
    return set(name, std::to_string(value));
}

bool ShaderSource::is_set(std::string_view name) const {
    return find_value(name) != nullptr;
}

std::vector<std::string> ShaderSource::unset_placeholders() const {
    std::vector<std::string> unset;
    for (const std::string& name : m_placeholders) {
        if (!is_set(name)) {
            unset.push_back(name);
        }
    }
    return unset;
}

std::string ShaderSource::glsl() const {
    const std::vector<std::string> unset = unset_placeholders();
    if (!unset.empty()) {
        std::string names;
        for (const std::string& name : unset) {
            names += (names.empty() ? "${" : ", ${") + name + "}";
        }
        throw std::logic_error("ShaderSource: no value set for " + names);
    }

    std::string out;
    size_t copied = 0;
    for_each_placeholder(m_template, [&](size_t begin, size_t end, std::string_view name) {
        out.append(m_template, copied, begin - copied);
        out += *find_value(name);
        copied = end;
    });
    out.append(m_template, copied, std::string::npos);
    return out;
}

std::string ShaderSource::numbered(const std::string& text) {
    std::string out;
    size_t line = 1;
    size_t begin = 0;
    while (begin < text.size()) {
        size_t end = text.find('\n', begin);
        if (end == std::string::npos) {
            end = text.size();
        }
        out += std::to_string(line) + ": ";
        out.append(text, begin, end - begin);
        out += "\n";
        begin = end + 1;
        ++line;
    }
    return out;
}

const std::string* ShaderSource::find_value(std::string_view name) const {
    for (const auto& [existing, value] : m_values) {
        if (existing == name) {
            return &value;
        }
    }
    return nullptr;
}

} // namespace vat::gl
