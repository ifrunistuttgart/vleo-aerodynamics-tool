#include "actors.h"

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <vtkArrowSource.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkConeSource.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTriangle.h>
#include <vtkTubeFilter.h>
#include <vtkUnsignedCharArray.h>

#include "palette.h"

namespace vat::visualization::actors {
namespace {

// Some unit vector perpendicular to n. Which one does not matter: it only fixes where
// the curved arrow starts, not which way round it sweeps.
glm::vec3 AnyPerpendicular(const glm::vec3& n) {
    const glm::vec3 reference =
        std::abs(n.x) < 0.9f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    return glm::normalize(glm::cross(n, reference));
}

vtkSmartPointer<vtkActor> ActorFor(vtkAlgorithmOutput* port, const glm::vec3& color) {
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(port);

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(color.r, color.g, color.b);
    return actor;
}

} // namespace

vtkSmartPointer<vtkActor> Arrow(const glm::vec3& dir, float length, const glm::vec3& color,
    double relative_shaft_radius, double relative_tip_radius, double relative_tip_length) {
    const glm::vec3 x_axis(1.0f, 0.0f, 0.0f);
    const glm::vec3 unit_dir = glm::length(dir) > 0.0f ? glm::normalize(dir) : x_axis;

    auto arrow_source = vtkSmartPointer<vtkArrowSource>::New();
    arrow_source->SetShaftRadius(relative_shaft_radius);
    arrow_source->SetTipRadius(relative_tip_radius);
    arrow_source->SetTipLength(relative_tip_length);

    auto transform = vtkSmartPointer<vtkTransform>::New();
    transform->Scale(length, length, length);

    // vtkArrowSource always points along +x, so rotate that onto unit_dir.
    const float dot = std::clamp(glm::dot(x_axis, unit_dir), -1.0f, 1.0f);
    if (dot < 0.999999f) {
        if (dot > -0.999999f) {
            const glm::vec3 rot_axis = glm::normalize(glm::cross(x_axis, unit_dir));
            const float angle__deg = glm::degrees(std::acos(dot));
            transform->RotateWXYZ(angle__deg, rot_axis.x, rot_axis.y, rot_axis.z);
        } else {
            transform->RotateWXYZ(180.0, 0.0, 0.0, 1.0);
        }
    }

    auto transform_filter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    transform_filter->SetTransform(transform);
    transform_filter->SetInputConnection(arrow_source->GetOutputPort());

    return ActorFor(transform_filter->GetOutputPort(), color);
}

vtkSmartPointer<vtkActor> ArrowAt(const glm::vec3& tail__m, const glm::vec3& dir, float length,
    const glm::vec3& color, double relative_shaft_radius, double relative_tip_radius,
    double relative_tip_length) {
    auto actor = Arrow(dir, length, color, relative_shaft_radius, relative_tip_radius,
        relative_tip_length);
    actor->SetPosition(tail__m.x, tail__m.y, tail__m.z);
    return actor;
}

vtkSmartPointer<vtkActor> Sphere(const glm::vec3& center__m, float radius, const glm::vec3& color) {
    auto sphere = vtkSmartPointer<vtkSphereSource>::New();
    sphere->SetCenter(center__m.x, center__m.y, center__m.z);
    sphere->SetRadius(radius);
    sphere->SetThetaResolution(24);
    sphere->SetPhiResolution(24);

    return ActorFor(sphere->GetOutputPort(), color);
}

/*
 * Rotating u about axis_hat by t gives u*cos(t) + (axis_hat x u)*sin(t) (Rodrigues, with
 * u perpendicular to axis_hat), so increasing t traces the right-hand-rule direction and
 * the arrow head ends up on the positive end.
 */
void AddArcArrow(vtkRenderer* renderer, const glm::vec3& center__m, const glm::vec3& axis_hat,
    float radius, float tube_radius, const glm::vec3& color) {
    constexpr int kArcSamples = 64;
    constexpr float kSweep__rad = 1.5f * std::numbers::pi_v<float>; // 270 degrees

    const glm::vec3 u = AnyPerpendicular(axis_hat);
    const glm::vec3 v = glm::cross(axis_hat, u);

    auto points = vtkSmartPointer<vtkPoints>::New();
    auto poly_line = vtkSmartPointer<vtkPolyLine>::New();
    poly_line->GetPointIds()->SetNumberOfIds(kArcSamples);
    for (int i = 0; i < kArcSamples; ++i) {
        const float t = kSweep__rad * static_cast<float>(i) / static_cast<float>(kArcSamples - 1);
        const glm::vec3 p = center__m + radius * (std::cos(t) * u + std::sin(t) * v);
        points->InsertNextPoint(p.x, p.y, p.z);
        poly_line->GetPointIds()->SetId(i, i);
    }

    auto lines = vtkSmartPointer<vtkCellArray>::New();
    lines->InsertNextCell(poly_line);

    auto arc_polydata = vtkSmartPointer<vtkPolyData>::New();
    arc_polydata->SetPoints(points);
    arc_polydata->SetLines(lines);

    auto tube = vtkSmartPointer<vtkTubeFilter>::New();
    tube->SetInputData(arc_polydata);
    tube->SetRadius(tube_radius);
    tube->SetNumberOfSides(16);
    tube->CappingOn();
    renderer->AddActor(ActorFor(tube->GetOutputPort(), color));

    // Arrow head at the swept end, pointing along the tangent there.
    const glm::vec3 end_point =
        center__m + radius * (std::cos(kSweep__rad) * u + std::sin(kSweep__rad) * v);
    const glm::vec3 tangent =
        glm::normalize(-std::sin(kSweep__rad) * u + std::cos(kSweep__rad) * v);
    const float cone_height = tube_radius * 7.0f;
    const glm::vec3 cone_center = end_point + tangent * (cone_height * 0.5f);

    auto cone = vtkSmartPointer<vtkConeSource>::New();
    cone->SetHeight(cone_height);
    cone->SetRadius(tube_radius * 2.8f);
    cone->SetResolution(24);
    cone->SetDirection(tangent.x, tangent.y, tangent.z);
    cone->SetCenter(cone_center.x, cone_center.y, cone_center.z);
    renderer->AddActor(ActorFor(cone->GetOutputPort(), color));
}

void AddBodyAxes(vtkRenderer* renderer, float bounding_sphere_radius) {
    const float axis_length =
        bounding_sphere_radius > 0.0f ? bounding_sphere_radius * 1.5f : 1.5f;
    const glm::vec3 directions[] = {
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f),
    };
    for (int i = 0; i < 3; ++i) {
        renderer->AddActor(Arrow(directions[i], axis_length, kAxisColors[i], 0.003, 0.008, 0.022));
    }
}

std::vector<vtkSmartPointer<vtkPolyData>> PerMeshPolyData(IGeometryShadingData& geometry) {
    const std::span<const unsigned int> num_triangles_per_mesh =
        geometry.get_num_triangles_per_mesh();
    const std::span<const float> vertices = geometry.get_vertices();

    std::vector<vtkSmartPointer<vtkPolyData>> per_mesh;
    per_mesh.reserve(num_triangles_per_mesh.size());

    std::size_t triangle_offset = 0;
    for (const unsigned int mesh_triangles : num_triangles_per_mesh) {
        auto points = vtkSmartPointer<vtkPoints>::New();
        points->SetNumberOfPoints(static_cast<vtkIdType>(3 * mesh_triangles));

        auto triangles = vtkSmartPointer<vtkCellArray>::New();
        triangles->AllocateEstimate(static_cast<vtkIdType>(mesh_triangles), 3);

        for (unsigned int tri = 0; tri < mesh_triangles; ++tri) {
            for (int corner = 0; corner < 3; ++corner) {
                const std::size_t src =
                    9 * (triangle_offset + tri) + 3 * static_cast<std::size_t>(corner);
                points->SetPoint(static_cast<vtkIdType>(3 * tri + corner),
                    vertices[src + 0], vertices[src + 1], vertices[src + 2]);
            }
            auto triangle = vtkSmartPointer<vtkTriangle>::New();
            triangle->GetPointIds()->SetId(0, 3 * tri + 0);
            triangle->GetPointIds()->SetId(1, 3 * tri + 1);
            triangle->GetPointIds()->SetId(2, 3 * tri + 2);
            triangles->InsertNextCell(triangle);
        }

        auto polydata = vtkSmartPointer<vtkPolyData>::New();
        polydata->SetPoints(points);
        polydata->SetPolys(triangles);
        per_mesh.push_back(polydata);

        triangle_offset += mesh_triangles;
    }
    return per_mesh;
}

vtkSmartPointer<vtkPolyData> VisibilityColoredPolyData(IGeometryShadingData& geometry,
    const std::vector<float>& triangle_visibility,
    const glm::vec3& visible_color, const glm::vec3& hidden_color) {
    const unsigned int num_triangles = geometry.get_num_triangles();
    if (triangle_visibility.size() != num_triangles) {
        SPDLOG_ERROR("Visibility size mismatch (visibility={}, triangles={})",
            triangle_visibility.size(), num_triangles);
        throw std::invalid_argument("triangle_visibility size does not match number of triangles");
    }

    const std::span<const float> vertices = geometry.get_vertices();

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

        const float visibility =
            std::clamp(triangle_visibility[static_cast<std::size_t>(tri)], 0.0f, 1.0f);
        const glm::vec3 color = hidden_color + visibility * (visible_color - hidden_color);

        const unsigned char rgb[3] = {
            static_cast<unsigned char>(std::clamp(color.r * 255.0f, 0.0f, 255.0f)),
            static_cast<unsigned char>(std::clamp(color.g * 255.0f, 0.0f, 255.0f)),
            static_cast<unsigned char>(std::clamp(color.b * 255.0f, 0.0f, 255.0f)),
        };
        cell_colors->SetTypedTuple(tri, rgb);
    }

    auto polydata = vtkSmartPointer<vtkPolyData>::New();
    polydata->SetPoints(points);
    polydata->SetPolys(triangles);
    polydata->GetCellData()->SetScalars(cell_colors);
    return polydata;
}

vtkSmartPointer<vtkActor> MeshActor(vtkPolyData* polydata, const glm::vec3& color, double opacity) {
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polydata);
    mapper->ScalarVisibilityOff();

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(color.r, color.g, color.b);
    actor->GetProperty()->SetOpacity(opacity);
    return actor;
}

vtkSmartPointer<vtkActor> ScalarColoredMeshActor(vtkPolyData* polydata) {
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polydata);
    mapper->ScalarVisibilityOn();
    mapper->SetScalarModeToUseCellData();

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    return actor;
}

float MeanTriangleExtent(IGeometryShadingData& geometry) {
    const std::span<const float> areas = geometry.get_areas();
    if (areas.empty()) {
        return 0.0f;
    }
    double total__m2 = 0.0;
    for (const float area__m2 : areas) {
        total__m2 += area__m2;
    }
    return static_cast<float>(std::sqrt(total__m2 / static_cast<double>(areas.size())));
}

} // namespace vat::visualization::actors
