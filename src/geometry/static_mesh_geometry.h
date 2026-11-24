#pragma once
#include <memory>
#include <string>
#include <vector>
#include <span>
#include <glm/glm.hpp>
#include "Igeometry_shading_data.h"
#include "Igeometry_manipulator.h"
#include "mesh_data.h"

namespace vat::geometry {

/**
 * @brief A geometry whose meshes stay in their loaded pose.
 *
 * It implements both IGeometryShadingData and IGeometryManipulator. Assimp-loaded or in-memory
 * meshes (MeshData) are de-indexed into 9 floats per triangle, with one normal, centroid and
 * area per triangle and 1-based triangle IDs. Every model matrix is the identity, so the
 * transformed and untransformed getters return the same data. All three IGeometryManipulator
 * functions return -1 and leave the geometry unchanged; use RotatableMeshGeometry to turn
 * meshes. It is also the base class of RotatableMeshGeometry and the input type of remeshing
 * and write_obj(), which use get_mesh_data().
 */
class StaticMeshGeometry : public IGeometryShadingData, public IGeometryManipulator {
protected:
    std::vector<MeshData> m_mesh_data;
    std::vector<float> m_vertices;
    std::vector<std::uint32_t> m_triangle_ids;
    std::vector<float> m_normals;
    std::vector<float> m_areas;
    std::vector<float> m_centroids;
    std::vector<glm::mat4> m_model_matrices;
    std::vector<unsigned int> m_num_triangles_per_mesh;
    std::vector<std::string> m_mesh_names;
    unsigned int m_total_triangles;
    float m_bounding_sphere_radius;

public:
    /**
     * @brief Loads a geometry from a model file (see load_mesh_data()).
     * @param file Path to any format assimp reads.
     * @throws std::runtime_error if the file cannot be read or contains no triangles.
     */
    StaticMeshGeometry(std::string file);

    /**
     * @brief Builds a geometry from meshes already in memory, e.g. the output of remeshing.
     *
     * @param meshes One entry per mesh, in the order that defines each mesh_id. A mesh
     *               with an empty name is named "Mesh_<index>".
     * @throws std::invalid_argument if a mesh's positions or indices do not come in
     *         triples, or an index is out of range.
     */
    explicit StaticMeshGeometry(std::vector<MeshData> meshes);
    ~StaticMeshGeometry() = default;

    std::span<const float> get_transformed_vertices() override;
    std::span<const float> get_untransformed_vertices() override;
    std::span<const std::uint32_t> get_triangle_ids() override;
    std::span<const float> get_transformed_normals() override;
    std::span<const float> get_untransformed_normals() override;
    std::span<const float> get_areas() override;
    std::span<const float> get_transformed_centroids() override;
    std::span<const float> get_untransformed_centroids() override;
    std::span<const glm::mat4> get_model_matrices() override;
    std::span<const std::string> get_mesh_names() override;
    std::span<const unsigned int> get_num_triangles_per_mesh() override;
    const unsigned int get_num_triangles() override;
    float get_bounding_sphere_radius() override;

    /**
     * @brief The meshes this geometry was built from, as indexed triangle lists in their own
     * untransformed frame and with the final (possibly defaulted) names.
     */
    std::span<const MeshData> get_mesh_data() const;

    // IGeometryManipulator interface
    /** @brief Not implemented for a static geometry. @return Always -1. */
    int turn_mesh(int mesh_id, float angle__rad) override;
    /** @brief Not implemented for a static geometry; see RotatableMeshGeometry. @return Always -1. */
    int turn_mesh_around_axis(const int mesh_id, float angle__rad, const std::array<float, 3>& origin,
                              const std::array<float, 3>& axis) override;
    /** @brief Not implemented for a static geometry. @return Always -1. */
    int turn_meshes() override;
};

} // namespace vat::geometry
