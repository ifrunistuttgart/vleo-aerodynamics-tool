#pragma once
#include <memory>
#include <string>
#include <vector>
#include <span>
#include <glm/glm.hpp>
#include "Igeometry_shading_data.h"
#include "Igeometry_manipulator.h"


namespace vat {

/**
* A geometry whose meshes are fixed in place. It implements both the IGeometryShadingData and IGeometryManipulator interfaces. 
* Altough it implements the IGeometryManipulator interface, the turn_mesh and turn_mesh_around_axis functions do not actually change the geometry's configuration, since the meshes are static.
* This class is useful for testing and benchmarking purposes, as it provides a simple implementation of the geometry interfaces without the complexity of handling dynamic transformations.
*/
class StaticMeshGeometry: public IGeometryShadingData, public IGeometryManipulator {
protected:
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
	StaticMeshGeometry(std::string file);
    ~StaticMeshGeometry() = default;

    std::span<const float> get_vertices() override;
    std::span<const std::uint32_t> get_triangle_ids() override;
    std::span<const float> get_normals() override;
    std::span<const float> get_areas() override;
    std::span<const float> get_centroids() override;
    std::span<const glm::mat4> get_model_matrices() override;
    std::span<const std::string> get_mesh_names() override;
    std::span<const unsigned int> get_num_triangles_per_mesh() override;
    const unsigned int get_num_triangles() override;
    float get_bounding_sphere_radius() override;

	//IGeometryManipulator interface
    int turn_mesh(int mesh_id, float angle__rad) override;
    int turn_mesh_around_axis(const int mesh_id, float angle__rad, const std::array<float, 3>& origin, const std::array<float, 3>& axis) override;
    int turn_meshes() override;
};

} // namespace vat
