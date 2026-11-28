#pragma once
#include <memory>
#include <string>
#include <span>
#include <glm/glm.hpp>
#include "static_mesh_geometry.h"

namespace vat::geometry {

/**
 * @brief A geometry whose meshes can be turned about arbitrary hinge axes.
 *
 * It inherits from StaticMeshGeometry and overrides turn_mesh_around_axis(), which sets the
 * mesh's model matrix. The transformed getters return the geometry with the model matrices
 * applied; they are recomputed lazily, once per pose change, and written in place so earlier
 * spans stay valid. The untransformed getters return the data as loaded. turn_mesh() and
 * turn_meshes() are inherited unchanged and always return -1.
 */
class RotatableMeshGeometry : public StaticMeshGeometry {
protected:
    std::vector<float> m_transformed_vertices;
    std::vector<float> m_transformed_normals;
    std::vector<float> m_transformed_centroids;
    bool m_transforms_outdated = true;

public:
    /**
     * @brief Loads a geometry from a model file, with every mesh unturned.
     * @param file Path to any format assimp reads.
     * @throws std::runtime_error if the file cannot be read or contains no triangles.
     */
    RotatableMeshGeometry(std::string file);
    /**
     * @brief Builds a geometry from meshes already in memory, with every mesh unturned.
     * @param meshes One entry per mesh, as for StaticMeshGeometry(std::vector<MeshData>).
     * @throws std::invalid_argument if a mesh's positions or indices do not come in
     *         triples, or an index is out of range.
     */
    explicit RotatableMeshGeometry(std::vector<MeshData> meshes);
    ~RotatableMeshGeometry() = default;

    std::span<const float> get_transformed_vertices() override;

    std::span<const float> get_untransformed_vertices() override;

    std::span<const float> get_transformed_normals() override;

    std::span<const float> get_untransformed_normals() override;

    std::span<const float> get_transformed_centroids() override;

    std::span<const float> get_untransformed_centroids() override;

    float get_bounding_sphere_radius() override;

    /**
     * @brief Sets the pose of one mesh; see IGeometryManipulator::turn_mesh_around_axis().
     *
     * Replaces the mesh's model matrix with a rotation by angle__rad about the axis through
     * origin; the transformed data is recomputed on the next getter call.
     *
     * @return 0 on success, -1 (with an error log) if mesh_id is out of range.
     */
    int turn_mesh_around_axis(const int mesh_id, float angle__rad, const std::array<float, 3>& origin,
                              const std::array<float, 3>& axis) override;

private:
    /**
     * @brief Recomputes the transformed geometry and the bounding sphere, but only if a mesh
     * has been turned since the last call.
     *
     * All three arrays and the radius are refreshed together because one force/torque
     * evaluation needs all of them and they all derive from the same model matrices.
     * Sweeping the flow direction at a fixed pose - the toolbox's main use case -
     * therefore pays for the transform once instead of on every getter call.
     */
    void refresh_transforms();

    /**
     * @brief Applies each mesh's model matrix to position data (vertices, centroids).
     *
     * @param coordinates Untransformed coordinates, mesh blocks in load order.
     * @param num_entries_per_triangle Floats one triangle occupies (9 for vertices, 3 for centroids).
     * @param target Pre-sized destination, written in place so a span handed out by an
     *               earlier getter call stays valid across a refresh.
     */
    void apply_transform(std::span<const float> coordinates, int num_entries_per_triangle,
                         std::vector<float>& target) const;

    /**
     * @brief Applies each mesh's normal matrix to direction data (normals).
     *
     * Uses the inverse transpose of the linear block, which stays correct if a
     * non-uniform scale is ever introduced, then renormalizes: Sentman divides only by
     * |v_rel| and so requires unit normals.
     *
     * @param normals Untransformed normals, mesh blocks in load order.
     * @param num_entries_per_triangle Floats one triangle occupies (3 for normals).
     * @param target Pre-sized destination, written in place. See apply_transform.
     */
    void apply_normal_transform(std::span<const float> normals, int num_entries_per_triangle,
                                std::vector<float>& target) const;
};

} // namespace vat::geometry
