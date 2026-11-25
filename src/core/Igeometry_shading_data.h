#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <glm/glm.hpp>

namespace vat {

/**
 * @brief Interface for providing geometric and shading-related data of the geometry.
 *
 * This data is used for aerodynamic load calculations and visibility analysis.
 * It includes vertex positions, triangle IDs, normals, and physical properties
 * like areas and centroids of the geometry's triangles.
 *
 * All per-triangle arrays are in the same triangle order: mesh by mesh, in mesh order
 * (see get_num_triangles_per_mesh()). Vertex data is de-indexed, i.e. three explicit
 * vertices (9 floats) per triangle. The spans returned by StaticMeshGeometry and
 * RotatableMeshGeometry stay valid for as long as the geometry object lives.
 */
class IGeometryShadingData {
public:
    virtual ~IGeometryShadingData() = default;

    /**
     * @brief Retrieves the current vertex positions, with each mesh's model matrix applied.
     *
     * For consumer code that needs the transformed geometry on the CPU.
     *
     * @return Vertex positions in the body frame [m], 9 floats (three x, y, z vertices) per triangle.
     */
    virtual std::span<const float> get_transformed_vertices() = 0;

    /**
     * @brief Retrieves the untransformed vertex positions of the underlying mesh data.
     *
     * This is used by GPU-based shading pipelines, which apply model matrices during draw
     * calls and must not receive geometry that is already model-transformed.
     *
     * @return Vertex positions in each mesh's own frame [m], 9 floats per triangle.
     */
    virtual std::span<const float> get_untransformed_vertices() = 0;

    /**
     * @brief Retrieves the triangle ID of every vertex.
     *
     * This is not an index buffer: it is a per-vertex attribute that carries the ID of the
     * triangle the vertex belongs to, so each ID appears three times in a row, once per
     * de-indexed vertex. IDs are 1-based (triangle i has ID i + 1), so 0 can mean background
     * in an ID image or visibility buffer.
     *
     * @return One triangle ID per vertex, 3 * get_num_triangles() entries.
     */
    virtual std::span<const std::uint32_t> get_triangle_ids() = 0;

    /**
     * @brief Retrieves the triangle normals, with each mesh's model matrix applied.
     *
     * @return One unit normal per triangle (x, y, z triplets), in the body frame.
     */
    virtual std::span<const float> get_transformed_normals() = 0;

    /**
     * @brief Retrieves the untransformed normals of the triangles, the counterpart of
     * get_untransformed_vertices().
     *
     * GPU pipelines that apply the model matrices themselves need these; uploading
     * get_transformed_normals() would rotate the normals twice.
     *
     * @return One normal per triangle (x, y, z triplets), in each mesh's own frame.
     */
    virtual std::span<const float> get_untransformed_normals() = 0;

    /**
     * @brief Retrieves the areas of the triangles in the geometry's meshes.
     *
     * @return The area of each triangle [m^2], one entry per triangle.
     */
    virtual std::span<const float> get_areas() = 0;

    /**
     * @brief Retrieves the triangle centroids, with each mesh's model matrix applied.
     *
     * @return One centroid per triangle (x, y, z triplets), in the body frame [m].
     */
    virtual std::span<const float> get_transformed_centroids() = 0;

    /**
     * @brief Retrieves the untransformed centroids of the triangles in the geometry meshes.
     *
     * @return One centroid per triangle (x, y, z triplets), in each mesh's own frame [m].
     */
    virtual std::span<const float> get_untransformed_centroids() = 0;

    /**
     * @brief Retrieves the 4x4 transformation (model) matrix of each mesh.
     *
     * Each matrix maps the mesh's untransformed coordinates into the body frame and defines
     * the current position and rotation of that mesh.
     *
     * @return One model matrix per mesh, in mesh order.
     */
    virtual std::span<const glm::mat4> get_model_matrices() = 0;

    /**
     * @brief Retrieves the name of each mesh of the geometry, as stored in the model file.
     *
     * Meshes without a name in the file fall back to "Mesh_<index>", so the returned
     * span always has one non-empty entry per mesh, in the same order as
     * get_num_triangles_per_mesh() and get_model_matrices(). The index into this span
     * is the mesh_id accepted by IGeometryManipulator::turn_mesh_around_axis().
     *
     * @return A span containing the name of each mesh.
     */
    virtual std::span<const std::string> get_mesh_names() = 0;

    /**
     * @brief Retrieves the number of triangles in each individual mesh of the geometry.
     *
     * @return The triangle count per mesh, in mesh order.
     */
    virtual std::span<const unsigned int> get_num_triangles_per_mesh() = 0;

    /**
     * @brief Retrieves the total number of triangles across all meshes of the geometry.
     *
     * @return The total triangle count.
     */
    virtual const unsigned int get_num_triangles() = 0;

    /**
     * @brief Retrieves the radius of a sphere centred at the body-frame origin that encloses
     * the geometry in its current pose.
     *
     * The sphere is centred at the origin, not at the geometry's centre: the radius is the
     * largest distance of any transformed vertex from the origin. The flow camera relies on
     * this (see gl::make_flow_camera()).
     *
     * @return The bounding sphere radius [m].
     */
    virtual float get_bounding_sphere_radius() = 0;
};

} // namespace vat
