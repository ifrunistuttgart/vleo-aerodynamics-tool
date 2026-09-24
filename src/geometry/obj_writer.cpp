#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include <cctype>
#include <charconv>
#include <fstream>
#include <stdexcept>
#include <system_error>

#include "obj_writer.h"

namespace vat::geometry {
namespace {

// Nine significant digits, the most a float needs. Assimp's .obj parser is not correctly
// rounded, so no spelling reads back bit-exact for every float; measured over 2e5 values,
// this one misses least and never by more than one ulp. to_chars rather than printf so a
// comma-decimal locale (e.g. a MATLAB session) cannot change the output.
void AppendFloat(std::string& line, float value) {
    char buffer[32];
    const auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), value,
        std::chars_format::general, 9);
    if (error != std::errc()) {
        throw std::runtime_error("could not format coordinate");
    }
    line.append(buffer, end);
}

// An `o` name ends at the first whitespace when the file is read back.
std::string ObjName(const std::string& name) {
    std::string sanitized = name;
    for (char& c : sanitized) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            c = '_';
        }
    }
    return sanitized;
}

} // namespace

void write_obj(StaticMeshGeometry& geometry, const std::string& file) {
    const auto meshes = geometry.get_mesh_data();
    if (geometry.get_num_triangles() == 0) {
        throw std::invalid_argument("cannot write " + file + ": the geometry has no triangles");
    }
    for (std::size_t i = 0; i < meshes.size(); ++i) {
        if (meshes[i].indices.empty()) {
            throw std::invalid_argument("cannot write " + file + ": mesh " + std::to_string(i)
                + " (\"" + meshes[i].name + "\") has no triangles and would disappear on reload, "
                "shifting every later mesh_id");
        }
    }
    const auto model_matrices = geometry.get_model_matrices();
    for (std::size_t i = 0; i < model_matrices.size(); ++i) {
        if (model_matrices[i] != glm::mat4(1.0f)) {
            throw std::invalid_argument("cannot write " + file + ": mesh " + std::to_string(i)
                + " (\"" + meshes[i].name + "\") is turned away from its original pose. "
                "Turn it back to angle 0 first; the file must hold the unturned geometry so "
                "hinge angles still apply to it");
        }
    }

    std::ofstream out(file, std::ios::binary);
    if (!out) {
        throw std::runtime_error("cannot open " + file + " for writing");
    }

    out << "# Written by the VLEO Aerodynamics Tool\n"
        << "# " << meshes.size() << " meshes, " << geometry.get_num_triangles() << " triangles\n";

    // .obj indices are 1-based and global across the whole file.
    std::size_t vertex_offset = 1;
    std::string line;
    for (const MeshData& mesh : meshes) {
        const std::string name = ObjName(mesh.name);
        if (name != mesh.name) {
            SPDLOG_WARN("Mesh \"{}\" is written as \"{}\": .obj object names cannot contain whitespace",
                mesh.name, name);
        }
        out << "o " << name << '\n';
        for (std::size_t v = 0; v < mesh.positions.size(); v += 3) {
            line = "v ";
            AppendFloat(line, mesh.positions[v]);
            line += ' ';
            AppendFloat(line, mesh.positions[v + 1]);
            line += ' ';
            AppendFloat(line, mesh.positions[v + 2]);
            line += '\n';
            out << line;
        }
        for (std::size_t t = 0; t < mesh.indices.size(); t += 3) {
            out << "f " << vertex_offset + mesh.indices[t]
                << ' ' << vertex_offset + mesh.indices[t + 1]
                << ' ' << vertex_offset + mesh.indices[t + 2] << '\n';
        }
        vertex_offset += mesh.positions.size() / 3;
    }

    out.flush();
    if (!out) {
        throw std::runtime_error("failed while writing " + file);
    }
    SPDLOG_INFO("Wrote {} meshes, {} triangles to {}", meshes.size(), geometry.get_num_triangles(), file);
}

} // namespace vat::geometry
