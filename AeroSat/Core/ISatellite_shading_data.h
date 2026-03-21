#pragma once
#include <cstdint>
#include <span>
#include <glm/glm.hpp>


class ISatelliteShadingData {
public:
    virtual ~ISatelliteShadingData() = default;

	// each vertex has three values (x, y, z)
    virtual std::span<const float> get_vertices() = 0;

    // one per vertex
    virtual  std::span<const std::uint32_t> get_triangle_ids() = 0;

	// three values per triangle 
    virtual std::span<const float> get_normals() = 0;

    // one value per triangle
	virtual std::span<const float> get_areas() = 0;

	// three values per triangle
	virtual std::span<const float> get_centroids() = 0;

	virtual std::span<const glm::mat4> get_model_matrices() = 0;
	virtual std::span<const unsigned int> get_num_triangles_per_mesh() = 0;
    virtual const unsigned int get_num_triangles() = 0;
    virtual float get_bounding_sphere_radius() = 0;
};