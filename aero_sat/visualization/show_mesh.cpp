#include "show_mesh.h"
//
#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include <vtkActor.h>
#include <vtkArrowSource.h>
#include <vtkAxesActor.h>
#include <vtkCellArray.h>
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkConeSource.h>
#include <vtkCoordinate.h>
#include <vtkTextActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>
#include <vtkTextProperty.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTriangle.h>
#include <vtkTubeFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkAutoInit.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

namespace vat {
namespace {

const glm::vec3 kBackgroundColor(0.08f, 0.08f, 0.1f);
constexpr int kWindowWidth = 1200;
constexpr int kWindowHeight = 900;

// Opacity of the meshes in ShowHinges(). Every mesh is translucent there, including the
// ones a hinge belongs to: a hinge normally sits inside its own mesh, so drawing that
// mesh opaque would hide the very markers the view exists to show. Each hinge is drawn
// in the colour of the mesh it turns, which is what ties the two together instead.
constexpr double kHingeViewMeshOpacity = 0.3;

// Okabe-Ito qualitative palette, which stays distinguishable under the common forms of
// colour blindness. The original palette's black is replaced by a light grey, because
// black is invisible against kBackgroundColor.
const glm::vec3 kMeshPalette[] = {
    glm::vec3(0.902f, 0.624f, 0.000f), // orange
    glm::vec3(0.337f, 0.706f, 0.914f), // sky blue
    glm::vec3(0.000f, 0.620f, 0.451f), // bluish green
    glm::vec3(0.941f, 0.894f, 0.259f), // yellow
    glm::vec3(0.000f, 0.447f, 0.698f), // blue
    glm::vec3(0.835f, 0.369f, 0.000f), // vermillion
    glm::vec3(0.800f, 0.475f, 0.655f), // reddish purple
    glm::vec3(0.700f, 0.700f, 0.700f), // light grey
};
constexpr std::size_t kMeshPaletteSize = std::size(kMeshPalette);

// Colour of mesh number mesh_index. Wraps around for geometries with more meshes than
// the palette holds; the repeats stay usable because the legend spells out the id.
glm::vec3 MeshColor(std::size_t mesh_index) {
    return kMeshPalette[mesh_index % kMeshPaletteSize];
}

// Every window is built the same way, and the widgets have to stay alive for as long as
// the window does, so they are kept together instead of dangling out of a factory call.
struct Scene {
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkRenderWindow> render_window;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor;
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> interaction_style;
    vtkSmartPointer<vtkAxesActor> axes;
    vtkSmartPointer<vtkOrientationMarkerWidget> orientation_widget;
};

Scene CreateScene(const std::string& window_name, bool enable_depth_peeling = false) {
    Scene scene;
    scene.renderer = vtkSmartPointer<vtkRenderer>::New();
    scene.render_window = vtkSmartPointer<vtkRenderWindow>::New();
    scene.interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    scene.interaction_style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();

    scene.interactor->SetInteractorStyle(scene.interaction_style);
    scene.render_window->AddRenderer(scene.renderer);
    scene.interactor->SetRenderWindow(scene.render_window);

    if (enable_depth_peeling) {
        // Order-independent transparency. Needs an alpha buffer and no MSAA; if the
        // driver cannot provide it VTK silently falls back to plain alpha blending.
        scene.render_window->SetAlphaBitPlanes(1);
        scene.render_window->SetMultiSamples(0);
        scene.renderer->SetUseDepthPeeling(1);
        scene.renderer->SetMaximumNumberOfPeels(8);
        scene.renderer->SetOcclusionRatio(0.05);
    }

    scene.axes = vtkSmartPointer<vtkAxesActor>::New();
    scene.orientation_widget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    scene.orientation_widget->SetOrientationMarker(scene.axes);
    scene.orientation_widget->SetInteractor(scene.interactor);
    scene.orientation_widget->SetViewport(0.0, 0.0, 0.25, 0.25);
    scene.orientation_widget->SetEnabled(1);
    scene.orientation_widget->InteractiveOn();

    scene.renderer->SetBackground(kBackgroundColor.r, kBackgroundColor.g, kBackgroundColor.b);
    scene.render_window->SetSize(kWindowWidth, kWindowHeight);
    scene.render_window->SetWindowName(window_name.c_str());
    return scene;
}

// Shows the window and blocks until the user closes it.
void RunScene(Scene& scene) {
    scene.render_window->Render();

    // VTK's default camera looks straight down -z, which shows a satellite edge-on and
    // makes all three body axes hard to tell apart. Swing round to a three-quarter view
    // so the model reads as a solid and x, y and z all point somewhere distinct.
    scene.renderer->ResetCamera();
    vtkCamera* camera = scene.renderer->GetActiveCamera();
    camera->Azimuth(45.0);
    camera->Elevation(25.0);
    camera->OrthogonalizeViewUp();
    scene.renderer->ResetCamera();

    scene.render_window->Render();
    scene.interactor->Initialize();
    scene.interactor->Start();
}

vtkSmartPointer<vtkActor> CreateArrowActor(const glm::vec3& dir, float length, const glm::vec3& color,
    double relative_shaft_radius, double relative_tip_radius, double relative_tip_length) {
    const glm::vec3 x_axis(1.0f, 0.0f, 0.0f);
    glm::vec3 unit_dir = glm::length(dir) > 0.0f ? glm::normalize(dir) : x_axis;

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
}

// Arrow whose tail sits at tail__m rather than at the body-frame origin.
vtkSmartPointer<vtkActor> CreateArrowActorAt(const glm::vec3& tail__m, const glm::vec3& dir, float length,
    const glm::vec3& color, double relative_shaft_radius, double relative_tip_radius, double relative_tip_length) {
    auto actor = CreateArrowActor(dir, length, color, relative_shaft_radius, relative_tip_radius, relative_tip_length);
    actor->SetPosition(tail__m.x, tail__m.y, tail__m.z);
    return actor;
}

vtkSmartPointer<vtkActor> CreateSphereActor(const glm::vec3& center__m, float radius, const glm::vec3& color) {
    auto sphere = vtkSmartPointer<vtkSphereSource>::New();
    sphere->SetCenter(center__m.x, center__m.y, center__m.z);
    sphere->SetRadius(radius);
    sphere->SetThetaResolution(24);
    sphere->SetPhiResolution(24);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(sphere->GetOutputPort());

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(color.r, color.g, color.b);
    return actor;
}

// Some unit vector perpendicular to n. Which one does not matter: it only fixes where
// the curved arrow starts, not which way round it sweeps.
glm::vec3 AnyPerpendicular(const glm::vec3& n) {
    const glm::vec3 reference = std::abs(n.x) < 0.9f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    return glm::normalize(glm::cross(n, reference));
}

// Draws a curved arrow wrapping axis_hat, sweeping the way a positive angle turns the
// mesh. Rotating u about axis_hat by t gives u*cos(t) + (axis_hat x u)*sin(t) (Rodrigues,
// with u perpendicular to axis_hat), so increasing t traces the right-hand-rule
// direction and the arrow head ends up on the positive end.
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

    auto tube_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    tube_mapper->SetInputConnection(tube->GetOutputPort());

    auto tube_actor = vtkSmartPointer<vtkActor>::New();
    tube_actor->SetMapper(tube_mapper);
    tube_actor->GetProperty()->SetColor(color.r, color.g, color.b);
    renderer->AddActor(tube_actor);

    // Arrow head at the swept end, pointing along the tangent there.
    const glm::vec3 end_point = center__m + radius * (std::cos(kSweep__rad) * u + std::sin(kSweep__rad) * v);
    const glm::vec3 tangent = glm::normalize(-std::sin(kSweep__rad) * u + std::cos(kSweep__rad) * v);
    const float cone_height = tube_radius * 7.0f;
    const glm::vec3 cone_center = end_point + tangent * (cone_height * 0.5f);

    auto cone = vtkSmartPointer<vtkConeSource>::New();
    cone->SetHeight(cone_height);
    cone->SetRadius(tube_radius * 2.8f);
    cone->SetResolution(24);
    cone->SetDirection(tangent.x, tangent.y, tangent.z);
    cone->SetCenter(cone_center.x, cone_center.y, cone_center.z);

    auto cone_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    cone_mapper->SetInputConnection(cone->GetOutputPort());

    auto cone_actor = vtkSmartPointer<vtkActor>::New();
    cone_actor->SetMapper(cone_mapper);
    cone_actor->GetProperty()->SetColor(color.r, color.g, color.b);
    renderer->AddActor(cone_actor);
}

void AddBodyAxes(vtkRenderer* renderer, float bounding_sphere_radius) {
    const float axis_length = bounding_sphere_radius > 0.0f ? bounding_sphere_radius * 1.5f : 1.5f;
    renderer->AddActor(CreateArrowActor(glm::vec3(1.0f, 0.0f, 0.0f), axis_length, glm::vec3(1.0f, 0.0f, 0.0f), 0.003, 0.008, 0.022));
    renderer->AddActor(CreateArrowActor(glm::vec3(0.0f, 1.0f, 0.0f), axis_length, glm::vec3(0.0f, 1.0f, 0.0f), 0.003, 0.008, 0.022));
    renderer->AddActor(CreateArrowActor(glm::vec3(0.0f, 0.0f, 1.0f), axis_length, glm::vec3(0.0f, 0.5f, 1.0f), 0.003, 0.008, 0.022));
}

// One vtkPolyData per mesh, in mesh order. The vertex array is not indexed -- it holds
// 9 floats per triangle -- so each mesh owns a contiguous run of it, and the per-mesh
// triangle counts say where each run ends.
std::vector<vtkSmartPointer<vtkPolyData>> BuildPerMeshPolyData(IGeometryShadingData& geometry) {
    const std::span<const unsigned int> num_triangles_per_mesh = geometry.get_num_triangles_per_mesh();
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
                const std::size_t src = 9 * (triangle_offset + tri) + 3 * static_cast<std::size_t>(corner);
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

vtkSmartPointer<vtkActor> CreateMeshActor(vtkPolyData* polydata, const glm::vec3& color, double opacity) {
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polydata);
    mapper->ScalarVisibilityOff();

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(color.r, color.g, color.b);
    actor->GetProperty()->SetOpacity(opacity);
    return actor;
}

// "[<mesh_id>] <name>" for every mesh. mesh_id is both the index into the mesh arrays
// and the argument turn_mesh_around_axis() takes, so the legend doubles as a lookup
// table for rotating a mesh.
std::vector<std::string> MeshLabels(IGeometryShadingData& geometry) {
    const std::span<const std::string> names = geometry.get_mesh_names();
    std::vector<std::string> labels;
    labels.reserve(names.size());
    for (std::size_t i = 0; i < names.size(); ++i) {
        labels.push_back(std::format("[{}] {}", i, names[i]));
    }
    return labels;
}

// Beyond this many rows the legend starts to crowd the viewport, so it is truncated with
// a final "... and N more" row instead. Models are usually a handful of meshes, but an
// .obj that stores every face as its own object easily runs to dozens.
constexpr std::size_t kMaxLegendEntries = 14;
constexpr int kLegendFontSize = 17;
constexpr double kLegendRowHeight = 0.028; // normalised viewport
constexpr double kLegendTop = 0.955;       // normalised viewport
constexpr double kLegendLeft = 0.015;      // normalised viewport

/*
 * Legend in the top-left corner: one row per entry, drawn in that entry's colour so the
 * colour itself is the key and no separate swatch is needed.
 *
 * The rows are individual text actors rather than a vtkLegendBoxActor, which derives its
 * font size from the box it is given and neither wraps nor shrinks a label too wide for
 * it -- so a long label is silently clipped, and widening the box to fit only scales the
 * font up by the same amount.
 *
 * They sit on the left because the rows are left-aligned and grow rightwards into empty
 * space, so a label of any length stays on screen. Right-aligning instead would mean
 * knowing how wide each label renders, which depends on the font the platform happens to
 * supply. The bottom-left is taken by the orientation marker, hence the top.
 */
void AddLegend(vtkRenderer* renderer,
    const std::vector<std::string>& labels, const std::vector<glm::vec3>& colors) {
    const bool truncated = labels.size() > kMaxLegendEntries;
    const std::size_t shown = truncated ? kMaxLegendEntries - 1 : labels.size();

    std::vector<std::string> rows;
    std::vector<glm::vec3> row_colors;
    for (std::size_t i = 0; i < shown; ++i) {
        rows.push_back(labels[i]);
        row_colors.push_back(colors[i]);
    }
    if (truncated) {
        rows.push_back(std::format("... and {} more", labels.size() - shown));
        row_colors.push_back(glm::vec3(0.7f));
    }

    for (std::size_t row = 0; row < rows.size(); ++row) {
        auto actor = vtkSmartPointer<vtkTextActor>::New();
        actor->SetInput(rows[row].c_str());
        actor->SetTextScaleModeToNone(); // honour SetFontSize instead of fitting to a box
        actor->GetTextProperty()->SetFontSize(kLegendFontSize);
        actor->GetTextProperty()->SetColor(row_colors[row].r, row_colors[row].g, row_colors[row].b);
        actor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
        actor->GetPositionCoordinate()->SetValue(
            kLegendLeft, kLegendTop - kLegendRowHeight * static_cast<double>(row));
        renderer->AddActor2D(actor);
    }
}

} // namespace

void ShowShading(
     IGeometryShadingData& geometry,
     const std::vector<float>& triangle_visibility,
     const glm::vec3& v_rel__m_per_s) {
    const glm::vec3 kVisibleTriangleColor(0.0f, 1.0f, 0.0f);
    const glm::vec3 kNonVisibleTriangleColor(0.15f, 0.2f, 0.8f);

    const unsigned int num_triangles = geometry.get_num_triangles();
    if (triangle_visibility.size() != num_triangles) {
        SPDLOG_ERROR("ShowShading: visibility size mismatch (visibility={}, triangles={})",
            triangle_visibility.size(), num_triangles);
        throw std::invalid_argument("triangle_visibility size does not match number of triangles");
    }

    SPDLOG_DEBUG("ShowShading start (triangles={}, |v_rel|={})",
        num_triangles, glm::length(v_rel__m_per_s));

    std::span<const float> vertices = geometry.get_vertices();

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
            kNonVisibleTriangleColor + vis * (kVisibleTriangleColor - kNonVisibleTriangleColor);

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

    const float bsr = geometry.get_bounding_sphere_radius();
    const float wind_length = bsr > 0.0f ? bsr * 1.3f : 1.0f;
    const glm::vec3 wind_hat = glm::length(v_rel__m_per_s) > 0.0f
        ? glm::normalize(v_rel__m_per_s)
        : glm::vec3(1.0f, 0.0f, 0.0f);

    auto wind_actor = CreateArrowActor(wind_hat, wind_length * 1.2f, glm::vec3(1.0f, 0.85f, 0.1f),
        0.0045, 0.012, 0.030);

    Scene scene = CreateScene("AeroSat Mesh Visibility + Wind");
    scene.renderer->AddActor(mesh_actor);
    scene.renderer->AddActor(wind_actor);
    AddBodyAxes(scene.renderer, bsr);

    RunScene(scene);
}

void ShowMeshes(IGeometryShadingData& geometry) {
    const std::vector<std::string> labels = MeshLabels(geometry);
    SPDLOG_DEBUG("ShowMeshes start (meshes={}, triangles={})", labels.size(), geometry.get_num_triangles());

    if (labels.empty()) {
        SPDLOG_ERROR("ShowMeshes: geometry contains no meshes");
        throw std::invalid_argument("geometry contains no meshes");
    }

    const std::vector<vtkSmartPointer<vtkPolyData>> per_mesh = BuildPerMeshPolyData(geometry);

    Scene scene = CreateScene("AeroSat Meshes");

    std::vector<glm::vec3> colors;
    colors.reserve(per_mesh.size());
    for (std::size_t i = 0; i < per_mesh.size(); ++i) {
        const glm::vec3 color = MeshColor(i);
        colors.push_back(color);
        scene.renderer->AddActor(CreateMeshActor(per_mesh[i], color, 1.0));
    }

    AddBodyAxes(scene.renderer, geometry.get_bounding_sphere_radius());
    AddLegend(scene.renderer, labels, colors);

    RunScene(scene);
}

void ShowHinges(IGeometryShadingData& geometry, const std::vector<Hinge>& hinges) {
    const std::vector<std::string> mesh_labels = MeshLabels(geometry);
    const std::size_t num_meshes = mesh_labels.size();
    SPDLOG_DEBUG("ShowHinges start (meshes={}, hinges={})", num_meshes, hinges.size());

    if (num_meshes == 0) {
        SPDLOG_ERROR("ShowHinges: geometry contains no meshes");
        throw std::invalid_argument("geometry contains no meshes");
    }

    // Validate up front: a bad mesh_id here almost certainly means the same bad id would
    // have been handed to turn_mesh_around_axis(), which is the mistake this view exists
    // to catch, so say so rather than silently drawing a hinge attached to nothing.
    for (const Hinge& hinge : hinges) {
        if (hinge.mesh_id < 0 || static_cast<std::size_t>(hinge.mesh_id) >= num_meshes) {
            SPDLOG_ERROR("ShowHinges: invalid mesh_id={} (num_meshes={})", hinge.mesh_id, num_meshes);
            throw std::invalid_argument("hinge mesh_id is out of range");
        }
        if (glm::length(hinge.axis) <= 0.0f) {
            SPDLOG_ERROR("ShowHinges: hinge for mesh_id={} has a zero-length axis", hinge.mesh_id);
            throw std::invalid_argument("hinge axis must not be zero-length");
        }
    }

    const std::vector<vtkSmartPointer<vtkPolyData>> per_mesh = BuildPerMeshPolyData(geometry);

    // Depth peeling, because the whole model is translucent here and without it VTK
    // blends the meshes in actor order rather than depth order, which puts far surfaces
    // in front of near ones.
    Scene scene = CreateScene("AeroSat Hinges", /*enable_depth_peeling=*/true);

    // Every mesh translucent, so that no hinge can be swallowed by the mesh it belongs to.
    for (std::size_t i = 0; i < per_mesh.size(); ++i) {
        scene.renderer->AddActor(CreateMeshActor(per_mesh[i], MeshColor(i), kHingeViewMeshOpacity));
    }

    const float bsr = geometry.get_bounding_sphere_radius();
    const float scale = bsr > 0.0f ? bsr : 1.0f;
    const float marker_radius = scale * 0.025f;
    const float axis_length = scale * 0.9f;
    const float arc_radius = scale * 0.18f;
    const float arc_tube_radius = scale * 0.012f;

    for (const Hinge& hinge : hinges) {
        const glm::vec3 color = MeshColor(static_cast<std::size_t>(hinge.mesh_id));
        const glm::vec3 axis_hat = glm::normalize(hinge.axis);

        // The rotation axis is a line, not a ray, so the arrow straddles the hinge point
        // and only the head marks which end is positive.
        const glm::vec3 tail = hinge.origin__m - axis_hat * (axis_length * 0.5f);

        scene.renderer->AddActor(CreateSphereActor(hinge.origin__m, marker_radius, color));
        scene.renderer->AddActor(CreateArrowActorAt(tail, axis_hat, axis_length, color, 0.004, 0.011, 0.028));
        AddArcArrow(scene.renderer, hinge.origin__m, axis_hat, arc_radius, arc_tube_radius, color);
    }

    AddBodyAxes(scene.renderer, bsr);

    // Legend lists the hinges, not every mesh, so it stays short and says which mesh
    // each hinge belongs to and where it sits.
    std::vector<std::string> legend_labels;
    std::vector<glm::vec3> legend_colors;
    legend_labels.reserve(hinges.size());
    legend_colors.reserve(hinges.size());
    for (const Hinge& hinge : hinges) {
        const std::size_t mesh_index = static_cast<std::size_t>(hinge.mesh_id);
        legend_labels.push_back(std::format("{} @ ({:.3g}, {:.3g}, {:.3g})",
            mesh_labels[mesh_index], hinge.origin__m.x, hinge.origin__m.y, hinge.origin__m.z));
        legend_colors.push_back(MeshColor(mesh_index));
    }
    if (!legend_labels.empty()) {
        AddLegend(scene.renderer, legend_labels, legend_colors);
    }

    RunScene(scene);
}

} // namespace vat
