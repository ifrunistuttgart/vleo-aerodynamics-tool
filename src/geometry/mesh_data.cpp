#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "mesh_data.h"

namespace vat::geometry {
namespace {

/*
 * Warns when a model separates its parts with `g` groups instead of `o` objects. A file
 * whose parts are groups of box faces loads as one mesh per face, so every mesh_id then
 * addresses a fragment and rotating one tears a face off a part.
 *
 * Assimp keeps a group only as an empty node -- no meshes, no children -- which is what
 * this counts. Files using `o` alone, and formats without groups, have none.
 */
void WarnIfFileUsesGroups(const aiScene* scene, const std::string& file) {
    const aiNode* root = scene->mRootNode;
    unsigned int num_group_markers = 0;
    for (unsigned int i = 0; i < root->mNumChildren; ++i) {
        const aiNode* child = root->mChildren[i];
        if (child->mNumMeshes == 0 && child->mNumChildren == 0) {
            ++num_group_markers;
        }
    }

    if (num_group_markers == 0) {
        return;
    }

    SPDLOG_WARN("{} appears to separate its parts with groups: found {} group(s) but {} "
        "mesh(es). Do not use groups in your .obj export -- use the 'o' identifier to "
        "separate meshes. Each mesh is what turn_mesh_around_axis() rotates, so with "
        "groups every mesh_id addresses only a fragment of a part.",
        file, num_group_markers, scene->mNumMeshes);
}

} // namespace

std::vector<MeshData> load_mesh_data(const std::string& file) {
    SPDLOG_INFO("Loading file {}", file);
    Assimp::Importer importer;

    // Load the scene with post-processing flags
    const aiScene* scene = importer.ReadFile(file,
        aiProcess_Triangulate |           // Ensure all faces are triangles
        aiProcess_JoinIdenticalVertices // Join identical vertices
    );

    if (!scene || scene->mNumMeshes == 0) {
        SPDLOG_ERROR("Failed to load model: {}", file);
        if (importer.GetErrorString()) {
            SPDLOG_ERROR("ASSIMP Error: {}", importer.GetErrorString());
        }
        return {};
    }
    SPDLOG_DEBUG("Successfully loaded {} meshes", scene->mNumMeshes);
    WarnIfFileUsesGroups(scene, file);

    // One entry of scene->mMeshes is one mesh of the geometry, i.e. one `o` object of an
    // .obj, and is the unit turn_mesh_around_axis() rotates.
    std::vector<MeshData> meshes;
    meshes.reserve(scene->mNumMeshes);
    for (unsigned int mesh_idx = 0; mesh_idx < scene->mNumMeshes; ++mesh_idx) {
        const aiMesh* mesh = scene->mMeshes[mesh_idx];
        SPDLOG_DEBUG("Processing mesh {} (\"{}\") with {} faces and {} vertices",
            mesh_idx, mesh->mName.C_Str(), mesh->mNumFaces, mesh->mNumVertices);

        MeshData& data = meshes.emplace_back();
        data.name = mesh->mName.C_Str();

        data.positions.reserve(3 * mesh->mNumVertices);
        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            data.positions.push_back(mesh->mVertices[v].x);
            data.positions.push_back(mesh->mVertices[v].y);
            data.positions.push_back(mesh->mVertices[v].z);
        }

        data.indices.reserve(3 * mesh->mNumFaces);
        for (unsigned int face_idx = 0; face_idx < mesh->mNumFaces; ++face_idx) {
            const aiFace& face = mesh->mFaces[face_idx];
            // Triangulation can still leave point and line primitives behind.
            if (face.mNumIndices == 3) {
                data.indices.push_back(face.mIndices[0]);
                data.indices.push_back(face.mIndices[1]);
                data.indices.push_back(face.mIndices[2]);
            }
        }
    }
    return meshes;
}

} // namespace vat::geometry
