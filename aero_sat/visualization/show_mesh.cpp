#include "show_mesh.h"
//
#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkAxesActor.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkLineSource.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTriangle.h>
#include <vtkTubeFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkAutoInit.h>

#include <iostream>

VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

namespace vat {

void ShowMeshWithShadingAndWind(
     ISatelliteShadingData& satellite,
     const std::vector<float>& triangle_visibility,
     const glm::vec3& v_rel__m_per_s) {
    const glm::vec3 kVisibleSurfaceColor(0.0f, 1.0f, 0.0f);
    const glm::vec3 kNonVisibleSurfaceColor(0.15f, 0.2f, 0.8f);

    const unsigned int num_triangles = satellite.get_num_triangles();
    if (triangle_visibility.size() != num_triangles) {
        SPDLOG_ERROR("ShowMeshWithShadingAndWind: visibility size mismatch (visibility={}, triangles={})",
            triangle_visibility.size(), num_triangles);
        throw std::invalid_argument("triangle_visibility size does not match number of triangles");
    }

    SPDLOG_DEBUG("ShowMeshWithShadingAndWind start (triangles={}, |v_rel|={})",
        num_triangles, glm::length(v_rel__m_per_s));

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

    auto create_arrow_actor = [](const glm::vec3& dir, float length, const glm::vec3& color,
        double relative_shaft_radius, double relative_tip_radius, double relative_tip_length) {
            const glm::vec3 x_axis(1.0f, 0.0f, 0.0f);
            glm::vec3 unit_dir = glm::length(dir) > 0.0f ? glm::normalize(dir) : x_axis;

            auto arrow_source = vtkSmartPointer<vtkArrowSource>::New();
            arrow_source->SetShaftRadius(relative_shaft_radius);
            arrow_source->SetTipRadius(relative_tip_radius);
            arrow_source->SetTipLength(relative_tip_length);

            auto transform = vtkSmartPointer<vtkTransform>::New();
            transform->Scale(length, length, length);

            const float dot = std::clamp(glm::dot(x_axis, unit_dir), -1.0f, 1.0f);
            if (dot < 0.999999f) {
                if (dot > -0.999999f) {
                    glm::vec3 rot_axis = glm::normalize(glm::cross(x_axis, unit_dir));
                    const float angle_deg = glm::degrees(std::acos(dot));
                    transform->RotateWXYZ(angle_deg, rot_axis.x, rot_axis.y, rot_axis.z);
                }
                else {
                    transform->RotateWXYZ(180.0, 0.0, 0.0, 1.0);
                }
            }

            auto transform_filter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
            transform_filter->SetTransform(transform);
            transform_filter->SetInputConnection(arrow_source->GetOutputPort());

            auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            mapper->SetInputConnection(transform_filter->GetOutputPort());

            auto actor = vtkSmartPointer<vtkActor>::New();
            actor->SetMapper(mapper);
            actor->GetProperty()->SetColor(color.r, color.g, color.b);
            return actor;
        };

    const float axis_length = bsr > 0.0f ? bsr * 1.5f : 1.5f;
    const float axis_thickness = bsr > 0.0f ? bsr * 0.005f : 0.005f;
    const float tip_radius = axis_thickness * 2.5f;
    const float tip_length = axis_length * 0.15f;
    auto x_axis_actor = create_arrow_actor(glm::vec3(1.0f, 0.0f, 0.0f), axis_length, glm::vec3(1.0f, 0.0f, 0.0f),
        0.003, 0.008, 0.022);
    auto y_axis_actor = create_arrow_actor(glm::vec3(0.0f, 1.0f, 0.0f), axis_length, glm::vec3(0.0f, 1.0f, 0.0f),
        0.003, 0.008, 0.022);
    auto z_axis_actor = create_arrow_actor(glm::vec3(0.0f, 0.0f, 1.0f), axis_length, glm::vec3(0.0f, 0.5f, 1.0f),
        0.003, 0.008, 0.022);

    auto wind_actor = create_arrow_actor(wind_hat, wind_length * 1.2f, glm::vec3(1.0f, 0.85f, 0.1f),
        0.0045, 0.012, 0.030);

    auto renderer = vtkSmartPointer<vtkRenderer>::New();
    auto render_window = vtkSmartPointer<vtkRenderWindow>::New();
    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();

    auto interaction_style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactor->SetInteractorStyle(interaction_style);

    render_window->AddRenderer(renderer);
    interactor->SetRenderWindow(render_window);

    auto axes = vtkSmartPointer<vtkAxesActor>::New();
    auto orientation_widget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    orientation_widget->SetOrientationMarker(axes);
    orientation_widget->SetInteractor(interactor);
    orientation_widget->SetViewport(0.0, 0.0, 0.25, 0.25);
    orientation_widget->SetEnabled(1);
    orientation_widget->InteractiveOn();

    renderer->AddActor(mesh_actor);
    renderer->AddActor(wind_actor);
    renderer->AddActor(x_axis_actor);
    renderer->AddActor(y_axis_actor);
    renderer->AddActor(z_axis_actor);
    renderer->SetBackground(0.08, 0.08, 0.1);

    render_window->SetSize(1200, 900);
    render_window->SetWindowName("AeroSat Mesh Visibility + Wind");
    render_window->Render();

    renderer->ResetCamera();
    render_window->Render();

    interactor->Initialize();
    interactor->Start();
    std::cout << "ShowMeshWithShadingAndWind: (triangles=" << satellite.get_num_triangles()
    << ", |v_rel|=" << glm::length(v_rel__m_per_s) << ")\n";

}

} // namespace vat
