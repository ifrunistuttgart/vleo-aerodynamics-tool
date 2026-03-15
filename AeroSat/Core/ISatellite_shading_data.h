#pragma once
#include <cstddef>

class ISatelliteShadingData {
public:
    virtual ~ISatelliteShadingData() = default;

    virtual float* get_vertices() = 0;
    virtual size_t get_num_vertices() = 0;
    virtual unsigned int* get_triangle_ids() = 0;
    virtual size_t get_num_triangle_ids() = 0;
    virtual float get_bounding_sphere_radius() = 0;
};