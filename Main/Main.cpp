#include <vtk-9.3/vtkActor.h>
#include <vtk-9.3/vtkCellArray.h>
#include <vtk-9.3/vtkCellData.h>
#include <vtk-9.3/vtkLineSource.h>
#include <vtk-9.3/vtkPoints.h>
#include <vtk-9.3/vtkPolyData.h>
#include <vtk-9.3/vtkPolyDataMapper.h>
#include <vtk-9.3/vtkProperty.h>
#include <vtk-9.3/vtkRenderWindow.h>
#include <vtk-9.3/vtkRenderWindowInteractor.h>
#include <vtk-9.3/vtkRenderer.h>
#include <vtk-9.3/vtkSmartPointer.h>
#include <vtk-9.3/vtkTriangle.h>
#include <vtk-9.3/vtkTubeFilter.h>
#include <vtk-9.3/vtkUnsignedCharArray.h>
#include <vtk-9.3/vtkAutoInit.h>

VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

#include <iostream>
#include <memory>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <span>
#include <glm/glm.hpp>
#include "RotatableMeshSatellite.h"
#include "ShadingPipeline.h"
#include "ShadingAlgorithmFactory.h"
#include "Sentman.h"
#include "Hybrid_force_torque_calculator.h"
#include "Core/Core.h"
#include <filesystem>
#include <cmath>
#define _USE_MATH_DEFINES

// Get path relative to this source file
std::string GetPath(const std::string& filename) {
    std::filesystem::path source_file(__FILE__);
    std::filesystem::path data_file = source_file.parent_path() / filename;
    return data_file.string();
}

void ShowMeshWithShadingAndWind(
    RotatableMeshSatellite& satellite,
    const std::vector<float>& triangle_visibility,
    const glm::vec3& v_rel__m_per_s) {
    const glm::vec3 kVisibleSurfaceColor(0.0f, 1.0f, 0.0f);
    const glm::vec3 kNonVisibleSurfaceColor(0.15f, 0.2f, 0.8f);

    const unsigned int num_triangles = satellite.get_num_triangles();
    if (triangle_visibility.size() != num_triangles) {
        throw std::invalid_argument("triangle_visibility size does not match number of triangles");
    }

    std::span<const float> vertices = satellite.get_vertices();

    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(static_cast<vtkIdType>(3 * num_triangles));

    for (vtkIdType i = 0; i < static_cast<vtkIdType>(3 * num_triangles); ++i) {
        const vtkIdType src = i * 3;
        points->SetPoint(i, vertices[src + 0], vertices[src + 1], vertices[src + 2]);
    }

    auto triangles = vtkSmartPointer<vtkCellArray>::New();
    triangles->AllocateEstimate(static_cast<vtkIdType>(num_triangles), 3);

    auto cell_colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    cell_colors->SetName("VisibilityColors");
    cell_colors->SetNumberOfComponents(3);
    cell_colors->SetNumberOfTuples(static_cast<vtkIdType>(num_triangles));

    for (vtkIdType tri = 0; tri < static_cast<vtkIdType>(num_triangles); ++tri) {
        auto triangle = vtkSmartPointer<vtkTriangle>::New();
        triangle->GetPointIds()->SetId(0, 3 * tri + 0);
        triangle->GetPointIds()->SetId(1, 3 * tri + 1);
        triangle->GetPointIds()->SetId(2, 3 * tri + 2);
        triangles->InsertNextCell(triangle);

        const float vis = std::clamp(triangle_visibility[static_cast<size_t>(tri)], 0.0f, 1.0f);
        const glm::vec3 color =
            kNonVisibleSurfaceColor + vis * (kVisibleSurfaceColor - kNonVisibleSurfaceColor);

        unsigned char rgb[3] = {
            static_cast<unsigned char>(std::clamp(color.x * 255.0f, 0.0f, 255.0f)),
            static_cast<unsigned char>(std::clamp(color.y * 255.0f, 0.0f, 255.0f)),
            static_cast<unsigned char>(std::clamp(color.z * 255.0f, 0.0f, 255.0f))
        };
        cell_colors->SetTypedTuple(tri, rgb);
    }

    auto mesh_polydata = vtkSmartPointer<vtkPolyData>::New();
    mesh_polydata->SetPoints(points);
    mesh_polydata->SetPolys(triangles);
    mesh_polydata->GetCellData()->SetScalars(cell_colors);

    auto mesh_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mesh_mapper->SetInputData(mesh_polydata);
    mesh_mapper->ScalarVisibilityOn();
    mesh_mapper->SetScalarModeToUseCellData();

    auto mesh_actor = vtkSmartPointer<vtkActor>::New();
    mesh_actor->SetMapper(mesh_mapper);

    const float bsr = satellite.get_bounding_sphere_radius();
    const float wind_length = bsr > 0.0f ? bsr * 1.3f : 1.0f;
    const glm::vec3 wind_hat = glm::length(v_rel__m_per_s) > 0.0f
        ? glm::normalize(v_rel__m_per_s)
        : glm::vec3(1.0f, 0.0f, 0.0f);

    auto wind_line = vtkSmartPointer<vtkLineSource>::New();
    wind_line->SetPoint1(0.0, 0.0, 0.0);
    wind_line->SetPoint2(
        wind_hat.x * wind_length,
        wind_hat.y * wind_length,
        wind_hat.z * wind_length);

    auto wind_tube = vtkSmartPointer<vtkTubeFilter>::New();
    wind_tube->SetInputConnection(wind_line->GetOutputPort());
    wind_tube->SetRadius(static_cast<double>(wind_length) * 0.01);
    wind_tube->SetNumberOfSides(16);

    auto wind_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    wind_mapper->SetInputConnection(wind_tube->GetOutputPort());

    auto wind_actor = vtkSmartPointer<vtkActor>::New();
    wind_actor->SetMapper(wind_mapper);
    wind_actor->GetProperty()->SetColor(1.0, 0.2, 0.2);

    auto renderer = vtkSmartPointer<vtkRenderer>::New();
    auto render_window = vtkSmartPointer<vtkRenderWindow>::New();
    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();

    render_window->AddRenderer(renderer);
    interactor->SetRenderWindow(render_window);

    renderer->AddActor(mesh_actor);
    renderer->AddActor(wind_actor);
    renderer->SetBackground(0.08, 0.08, 0.1);

    render_window->SetSize(1200, 900);
    render_window->SetWindowName("AeroSat Mesh Visibility + Wind");
    render_window->Render();

    renderer->ResetCamera();
    render_window->Render();

    interactor->Initialize();
    interactor->Start();

}

int main() {
    try {
        std::cout << "=== AeroSat Aerodynamic Force & Torque Calculator ===" << std::endl;
        std::cout << std::endl;

        // ============================================================================
        // STEP 1: Load Satellite Model
        // ============================================================================
        std::cout << "[1] Loading satellite model..." << std::endl;

        std::string satellite_path = GetPath("ISS_cut_no_materials.obj");
        std::unique_ptr<RotatableMeshSatellite> satellite =
            std::make_unique<RotatableMeshSatellite>(satellite_path);

        std::cout << "     Loaded " << satellite->get_num_triangles() << " triangles" << std::endl;


        // ============================================================================
        // STEP 2: Initialize Shading Pipeline (for shadow calculations)
        // ============================================================================
        std::cout << "[2] Initializing shading pipeline..." << std::endl;
        std::unique_ptr<ShadingPipeline> shading_pipeline =
            std::make_unique<ShadingPipeline>(*satellite, ShadingAlgorithmType::Binary, 5000);

        std::cout << "     Shading pipeline ready" << std::endl;

        // ============================================================================
        // STEP 3: Initialize Gas-Surface Interaction (GSI) Model
        // ============================================================================
        std::cout << "[3] Initializing GSI model (Sentman)..." << std::endl;
        std::unique_ptr<Sentman> gsi_model = std::make_unique<Sentman>(1);
        std::cout << "     Sentman GSI model initialized" << std::endl;
        std::cout << std::endl;

        // ============================================================================
        // STEP 4: Create Force & Torque Calculator
        // ============================================================================
        std::cout << "[4] Creating aerodynamic force/torque calculator..." << std::endl;
        std::unique_ptr<HybridForceTorqueCalculator> calculator =
            std::make_unique<HybridForceTorqueCalculator>(*satellite, *shading_pipeline, *gsi_model);
        std::cout << "     Calculator initialized" << std::endl;
        std::cout << std::endl;

        // ============================================================================
        // STEP 5: Define Orbital Conditions (Low Earth Orbit)
        // ============================================================================
        AeroConditions leo_conditions;
        leo_conditions.density__kg_per_m3 = 1.2482e-11f;      // Typical at 400 km altitude
        leo_conditions.temperature__K = 934.0f;                // Exospheric temperature
        leo_conditions.particle_mass__kg = 16 * 1.6605390689252e-27f;  // Atomic oxygen
        leo_conditions.alpha_e = 0.9f;                          // Energy accommodation coefficient

        // ============================================================================
        // STEP 6: Define Satellite State
        // ============================================================================
        glm::vec3 velocity__m_per_s(0.0f, -7800.0f, 0.0f);  // ~7.8 km/s orbital velocity
        float surface_temperature__K = 300.0f;                     // 300 K (~27°C)

        // ============================================================================
        // STEP 7: rotate mesh
        // ============================================================================
		//rotate one solar panel by 180 degrees around its hinge axis (x-axis)
        satellite->turn_surface_around_axis(1, -3.14159265358979323846f / 3.0f, { 0.0f, -0.1789f, 1.5f }, { 1.0f, 0.0f, 0.0f });
        satellite->turn_surface_around_axis(2, -3.14159265358979323846f / 3.0f, { 0.0f, -0.1789f, 1.5f }, { 1.0f, 0.0f, 0.0f });
        // ============================================================================
        // STEP 8: Calculate Aerodynamic Forces & Torques
        // ============================================================================
        std::cout << "[7] CALCULATING AERODYNAMIC FORCES & TORQUES..." << std::endl;
        std::cout << std::string(50, '-') << std::endl;

        glm::vec3 force__N(0.0f);
        glm::vec3 torque__Nm(0.0f);

        auto calc_start = std::chrono::high_resolution_clock::now();

        int result = 0;
        int cycles = 10;
        for (int i = 0; i < cycles; ++i) {
            int result = calculator->calc_aero_torque_force(
                velocity__m_per_s,
                surface_temperature__K,
                leo_conditions,
                torque__Nm,
                force__N
            );
        }
        auto calc_end = std::chrono::high_resolution_clock::now();
        auto calc_duration = std::chrono::duration_cast<std::chrono::microseconds>(calc_end - calc_start);

        std::vector<float> triangle_visibility(satellite->get_num_triangles(), 0.0f);
        auto shade_start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < cycles; ++i) {
            result = shading_pipeline->shade(std::span<float>(triangle_visibility), glm::normalize(velocity__m_per_s));
        }
        auto shade_end = std::chrono::high_resolution_clock::now();
        auto shade_duration = std::chrono::duration_cast<std::chrono::microseconds>(shade_end - shade_start);

        std::cout << std::string(50, '-') << std::endl;
        std::cout << std::endl;

        // ============================================================================
        // STEP 8: Display Results
        // ============================================================================
        if (result == 0) {
            std::cout << "[RESULTS]  Calculation successful" << std::endl;
            std::cout << std::endl;

            std::cout << "AERODYNAMIC FORCE (N):" << std::endl;
            std::cout << "    Fx: " << std::scientific << std::setprecision(6) << force__N.x << std::endl;
            std::cout << "    Fy: " << std::scientific << std::setprecision(6) << force__N.y << std::endl;
            std::cout << "    Fz: " << std::scientific << std::setprecision(6) << force__N.z << std::endl;
            std::cout << "    |F|: " << std::scientific << std::setprecision(6) << glm::length(force__N) << std::endl;
            std::cout << std::endl;

            std::cout << "AERODYNAMIC TORQUE (N·m):" << std::endl;
            std::cout << "    Tx: " << std::scientific << std::setprecision(6) << torque__Nm.x << std::endl;
            std::cout << "    Ty: " << std::scientific << std::setprecision(6) << torque__Nm.y << std::endl;
            std::cout << "    Tz: " << std::scientific << std::setprecision(6) << torque__Nm.z << std::endl;
            std::cout << "    |T|: " << std::scientific << std::setprecision(6) << glm::length(torque__Nm) << std::endl;
            std::cout << std::endl;

            std::cout << std::string(50, '=') << std::endl;
            std::cout << " TIMING BREAKDOWN:" << std::endl;
            std::cout << std::string(50, '=') << std::endl;
            std::cout << "    Force/Torque calc:       " << std::setw(8) << std::fixed << std::setprecision(3)
                << calc_duration.count()/cycles / 1000.0f << " ms average per force calculation" << std::endl;
            std::cout << std::string(50, '=') << std::endl;
            std::cout << "    Shading:       " << std::setw(8) << std::fixed << std::setprecision(3)
                << shade_duration.count() / cycles / 1000.0f << " ms average per shading calculation" << std::endl;
            std::cout << std::string(50, '=') << std::endl;

            int shade_result = shading_pipeline->shade(std::span<float>(triangle_visibility), glm::normalize(velocity__m_per_s));
			std::cout <<"shade result: " << shade_result << std::endl;
           
            if (shade_result == 0) {
                ShowMeshWithShadingAndWind(*satellite, triangle_visibility, velocity__m_per_s);
            }
            else {
                std::cout << "[Viewer] Shading failed with code: " << shade_result << std::endl;
            }
        }
        else {
            std::cout << "[ERROR] Calculation failed with code: " << result << std::endl;
            return 1;
        }

        std::cout << std::endl << "Program completed successfully!" << std::endl;
        return 0;

    }
    catch (const std::exception& e) {
        std::cerr << "[FATAL ERROR] " << e.what() << std::endl;
        return -1;
    }
}