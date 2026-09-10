#include "viewer.h"

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include <cctype>
#include <format>
#include <string>

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkCoordinate.h>
#include <vtkProperty.h>
#include <vtkTextProperty.h>

#include "actors.h"
#include "edge_style.h"
#include "palette.h"

namespace vat::visualization {
namespace {

/*
 * Beyond this many rows the legend starts to crowd the viewport, so it is truncated with
 * a final "... and N more" row instead. Models are usually a handful of meshes, but an
 * .obj that stores every face as its own object easily runs to dozens.
 */
constexpr std::size_t kMaxLegendEntries = 14;
constexpr int kLegendFontSize = 17;
constexpr double kLegendRowHeight = 0.028; // normalised viewport
constexpr double kLegendTop = 0.955;       // normalised viewport
constexpr double kLegendLeft = 0.015;      // normalised viewport

/*
 * The edge-overlay hint. Top-right, because the legend owns the top-left and the
 * orientation marker the bottom-left -- and bright enough to actually be read: an
 * earlier dim version in the bottom corner went unnoticed, which makes a keyboard-only
 * toggle useless.
 */
constexpr int kHintFontSize = 17;
const glm::vec3 kHintColor(0.80f, 0.80f, 0.84f);
constexpr double kHintRight = 0.985; // normalised viewport
constexpr double kHintTop = 0.955;   // normalised viewport

vtkSmartPointer<vtkTextActor> TextActor(const std::string& text, int font_size,
    const glm::vec3& color) {
    auto actor = vtkSmartPointer<vtkTextActor>::New();
    actor->SetInput(text.c_str());
    actor->SetTextScaleModeToNone(); // honour SetFontSize instead of fitting to a box
    actor->GetTextProperty()->SetFontSize(font_size);
    actor->GetTextProperty()->SetColor(color.r, color.g, color.b);
    actor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    return actor;
}

} // namespace

Viewer::Viewer(Config config) : m_config(std::move(config)) {
    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_render_window = vtkSmartPointer<vtkRenderWindow>::New();
    m_interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    m_interaction_style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();

    m_interactor->SetInteractorStyle(m_interaction_style);
    m_render_window->AddRenderer(m_renderer);
    m_interactor->SetRenderWindow(m_render_window);

    if (m_config.depth_peeling) {
        // Needs an alpha buffer and no MSAA; if the driver cannot provide it VTK
        // silently falls back to plain alpha blending.
        m_render_window->SetAlphaBitPlanes(1);
        m_render_window->SetMultiSamples(0);
        m_renderer->SetUseDepthPeeling(1);
        m_renderer->SetMaximumNumberOfPeels(8);
        m_renderer->SetOcclusionRatio(0.05);
    }

    m_axes = vtkSmartPointer<vtkAxesActor>::New();
    m_orientation_widget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    m_orientation_widget->SetOrientationMarker(m_axes);
    m_orientation_widget->SetInteractor(m_interactor);
    m_orientation_widget->SetViewport(0.0, 0.0, 0.25, 0.25);
    m_orientation_widget->SetEnabled(1);
    m_orientation_widget->InteractiveOn();

    m_edges.set_enabled(m_config.show_triangle_edges);

    InstallObservers();
    AddEdgeHint();

    m_renderer->SetBackground(kBackgroundColor.r, kBackgroundColor.g, kBackgroundColor.b);
    m_render_window->SetSize(kWindowWidth, kWindowHeight);
    m_render_window->SetWindowName(m_config.title.c_str());
}

void Viewer::InstallObservers() {
    m_key_callback = vtkSmartPointer<vtkCallbackCommand>::New();
    m_key_callback->SetCallback(OnKeyPress);
    m_key_callback->SetClientData(this);
    m_interactor->AddObserver(vtkCommand::KeyPressEvent, m_key_callback);

    m_render_callback = vtkSmartPointer<vtkCallbackCommand>::New();
    m_render_callback->SetCallback(OnStartRender);
    m_render_callback->SetClientData(this);
    m_renderer->AddObserver(vtkCommand::StartEvent, m_render_callback);
}

void Viewer::AddEdgeHint() {
    // Upper case in the label only: the key itself is matched case-insensitively, and a
    // lone lower-case letter in a sentence does not read as a key to press.
    std::string key_label(kEdgeToggleKey);
    for (char& c : key_label) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    m_edge_hint = TextActor(
        std::format("Press  {}  to toggle triangle edges", key_label), kHintFontSize,
        kHintColor);
    m_edge_hint->GetTextProperty()->SetJustificationToRight();
    m_edge_hint->GetPositionCoordinate()->SetValue(kHintRight, kHintTop);
    m_renderer->AddActor2D(m_edge_hint);
}

void Viewer::AddMesh(vtkPolyData* polydata, const glm::vec3& color, double opacity) {
    vtkSmartPointer<vtkActor> actor = actors::MeshActor(polydata, color, opacity);
    m_edges.Register(actor, color);
    m_renderer->AddActor(actor);
}

void Viewer::AddScalarColoredMesh(vtkPolyData* polydata, const glm::vec3& edge_fade_target) {
    vtkSmartPointer<vtkActor> actor = actors::ScalarColoredMeshActor(polydata);
    m_edges.Register(actor, edge_fade_target);
    m_renderer->AddActor(actor);
}

void Viewer::AddActor(vtkSmartPointer<vtkActor> actor) {
    m_renderer->AddActor(std::move(actor));
}

void Viewer::SetMeanTriangleExtent(float extent__m) {
    m_edges.set_mean_triangle_extent__m(extent__m);
}

void Viewer::AddBodyAxes(float bounding_sphere_radius) {
    actors::AddBodyAxes(m_renderer, bounding_sphere_radius);
}

/*
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
void Viewer::AddLegend(const std::vector<std::string>& labels,
    const std::vector<glm::vec3>& colors) {
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
        vtkSmartPointer<vtkTextActor> actor =
            TextActor(rows[row], kLegendFontSize, row_colors[row]);
        actor->GetPositionCoordinate()->SetValue(
            kLegendLeft, kLegendTop - kLegendRowHeight * static_cast<double>(row));
        m_renderer->AddActor2D(actor);
    }
}

/*
 * Open on the view an attitude-control engineer expects of a nadir-pointing spacecraft:
 * +x along the direction of travel, +y to the right of it, +z down towards the Earth.
 * Screen-up is therefore -z, which is what SetViewUp fixes -- VTK's default (+y up,
 * looking down -z) shows that frame lying on its side.
 *
 * The offset has to be behind (-x): from in front, a screen-up of -z places +y on the
 * left instead, inverting the convention this view exists to show.
 */
void Viewer::PlaceCamera() {
    vtkCamera* camera = m_renderer->GetActiveCamera();
    camera->SetFocalPoint(0.0, 0.0, 0.0);
    camera->SetPosition(-1.0, 0.55, -0.75); // -x behind, +y right, -z above
    camera->SetViewUp(0.0, 0.0, -1.0);
    camera->OrthogonalizeViewUp();

    // Slides the camera along that direction until the model fits, leaving the
    // orientation and the view-up alone.
    m_renderer->ResetCamera();
}

void Viewer::Run() {
    m_render_window->Render();
    PlaceCamera();
    m_render_window->Render();

    m_interactor->Initialize();
    m_interactor->Start();
}

void Viewer::OnKeyPress(vtkObject* caller, unsigned long, void* client_data, void*) {
    auto* interactor = static_cast<vtkRenderWindowInteractor*>(caller);
    auto* viewer = static_cast<Viewer*>(client_data);

    const char* key_sym = interactor->GetKeySym();
    if (key_sym == nullptr) {
        return;
    }
    const std::string key(key_sym);
    if (key != kEdgeToggleKey && key != "T") {
        return;
    }

    // Only flips the flag; OnStartRender applies it, so the key and the zoom fade never
    // fight over the same property.
    viewer->m_edges.Toggle();
    SPDLOG_DEBUG("Triangle edges toggled {}", viewer->m_edges.enabled() ? "on" : "off");
    interactor->GetRenderWindow()->Render();
}

void Viewer::OnStartRender(vtkObject* caller, unsigned long, void* client_data, void*) {
    auto* renderer = static_cast<vtkRenderer*>(caller);
    auto* viewer = static_cast<Viewer*>(client_data);
    viewer->m_edges.ApplyForCamera(renderer);
}

} // namespace vat::visualization
