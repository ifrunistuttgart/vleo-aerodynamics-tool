#pragma once
//potentially usefull for matlab bindings
//#ifdef _WIN32
//    #pragma comment(lib, "opengl32.lib")
//    #pragma comment(lib, "user32.lib")
//    #pragma comment(lib, "gdi32.lib")
//    #pragma comment(lib, "shell32.lib")
//#endif

#include <memory>
#include "core/core.h"
#include <string>
#include <vector>
#include "satellite/rotatable_mesh_satellite.h"

class RotatableMeshSatelliteMatlab;
class VleoAerodynamics;

// also satellite interface needs to be supported by matlab, so for this also a wrapper needs to be written since it uses glm too
class VleoAerodynamicsMatlab {
private:
    RotatableMeshSatelliteMatlab& m_satellite;
    std::unique_ptr<VleoAerodynamics> m_vleo_aerodynamics;

public:
    VleoAerodynamicsMatlab(RotatableMeshSatelliteMatlab& matlab_satellite, std::string gsi_model, int GPU_resolution);
    ~VleoAerodynamicsMatlab();

    int calculate_aero_torque_force(
        const std::vector<float>& velocity__m_per_s,
        float surface_temperature__K,
        AeroConditions aero,
        std::vector<float>& aero_torque__Nm,
        std::vector<float>& aero_force__N
    );
    //int visualize_shading(const glm::vec3& velocity__m_per_s);
};

class RotatableMeshSatelliteMatlab {
public:
    RotatableMeshSatelliteMatlab(std::string file);
    ~RotatableMeshSatelliteMatlab() = default;

    //ISatelliteManipulator interface
    int turn_surface_around_axis(const int surface_id, float angle__rad, const std::vector<float>& origin, const std::vector<float>& axis);

    RotatableMeshSatellite& get_satellite();

private:
    std::unique_ptr<RotatableMeshSatellite> m_rotatable_mesh_satellite;
};