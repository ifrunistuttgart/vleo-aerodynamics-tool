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
constexpr std::size_t MAX_LEGEND_ENTRIES = 14;
constexpr int LEGEND_FONT_SIZE = 17;
constexpr double LEGEND_ROW_HEIGHT = 0.028; // normalised viewport
constexpr double LEGEND_TOP = 0.955;        // normalised viewport
constexpr double LEGEND_LEFT = 0.015;       // normalised viewport

/*
 * The edge-overlay hint. Top-right, because the legend owns the top-left and the
 * orientation marker the bottom-left -- and bright enough to actually be read: an
 * earlier dim version in the bottom corner went unnoticed, which makes a keyboard-only
 * toggle useless.
 */
constexpr int HINT_FONT_SIZE = 17;
const glm::vec3 HINT_COLOR(0.80f, 0.80f, 0.84f);
constexpr double HINT_RIGHT = 0.985; // normalised viewport
constexpr double HINT_TOP = 0.955;   // normalised viewport

vtkSmartPointer<vtkTextActor> text_actor(const std::string& text, int font_size, const glm::vec3& color) {
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

    install_observers();
    add_edge_hint();

    m_renderer->SetBackground(BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b);
    m_render_window->SetSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    m_render_window->SetWindowName(m_config.title.c_str());
}

void Viewer::install_observers() {
    m_key_callback = vtkSmartPointer<vtkCallbackCommand>::New();
    m_key_callback->SetCallback(on_key_press);
    m_key_callback->SetClientData(this);
    m_interactor->AddObserver(vtkCommand::KeyPressEvent, m_key_callback);

    m_render_callback = vtkSmartPointer<vtkCallbackCommand>::New();
    m_render_callback->SetCallback(on_start_render);
    m_render_callback->SetClientData(this);
    m_renderer->AddObserver(vtkCommand::StartEvent, m_render_callback);
}

void Viewer::add_edge_hint() {
    // Upper case in the label only: the key itself is matched case-insensitively, and a
    // lone lower-case letter in a sentence does not read as a key to press.
    std::string key_label(EDGE_TOGGLE_KEY);
    for (char& c : key_label) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    m_edge_hint = text_actor(std::format("Press  {}  to toggle triangle edges", key_label), HINT_FONT_SIZE, HINT_COLOR);
    m_edge_hint->GetTextProperty()->SetJustificationToRight();
    m_edge_hint->GetPositionCoordinate()->SetValue(HINT_RIGHT, HINT_TOP);
    m_renderer->AddActor2D(m_edge_hint);
}

void Viewer::add_mesh(vtkPolyData* polydata, const glm::vec3& color, double opacity) {
    vtkSmartPointer<vtkActor> actor = actors::mesh_actor(polydata, color, opacity);
    m_edges.register_actor(actor, color);
    m_renderer->AddActor(actor);
}

void Viewer::add_scalar_colored_mesh(vtkPolyData* polydata, const glm::vec3& edge_fade_target) {
    vtkSmartPointer<vtkActor> actor = actors::scalar_colored_mesh_actor(polydata);
    m_edges.register_actor(actor, edge_fade_target);
    m_renderer->AddActor(actor);
}

void Viewer::add_actor(vtkSmartPointer<vtkActor> actor) { m_renderer->AddActor(std::move(actor)); }

void Viewer::set_mean_triangle_extent__m(float extent__m) { m_edges.set_mean_triangle_extent__m(extent__m); }

void Viewer::add_body_axes(float bounding_sphere_radius) { actors::add_body_axes(m_renderer, bounding_sphere_radius); }

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
void Viewer::add_legend(const std::vector<std::string>& labels, const std::vector<glm::vec3>& colors) {
    const bool truncated = labels.size() > MAX_LEGEND_ENTRIES;
    const std::size_t shown = truncated ? MAX_LEGEND_ENTRIES - 1 : labels.size();

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
        vtkSmartPointer<vtkTextActor> actor = text_actor(rows[row], LEGEND_FONT_SIZE, row_colors[row]);
        actor->GetPositionCoordinate()->SetValue(LEGEND_LEFT,
                                                 LEGEND_TOP - LEGEND_ROW_HEIGHT * static_cast<double>(row));
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
void Viewer::place_camera() {
    vtkCamera* camera = m_renderer->GetActiveCamera();
    camera->SetFocalPoint(0.0, 0.0, 0.0);
    camera->SetPosition(-1.0, 0.55, -0.75); // -x behind, +y right, -z above
    camera->SetViewUp(0.0, 0.0, -1.0);
    camera->OrthogonalizeViewUp();

    // Slides the camera along that direction until the model fits, leaving the
    // orientation and the view-up alone.
    m_renderer->ResetCamera();
}

void Viewer::run() {
    m_render_window->Render();
    place_camera();
    m_render_window->Render();

    m_interactor->Initialize();
    m_interactor->Start();
}

void Viewer::on_key_press(vtkObject* caller, unsigned long, void* client_data, void*) {
    auto* interactor = static_cast<vtkRenderWindowInteractor*>(caller);
    auto* viewer = static_cast<Viewer*>(client_data);

    const char* key_sym = interactor->GetKeySym();
    if (key_sym == nullptr) {
        return;
    }
    const std::string key(key_sym);
    if (key != EDGE_TOGGLE_KEY && key != "T") {
        return;
    }

    // Only flips the flag; on_start_render applies it, so the key and the zoom fade never
    // fight over the same property.
    viewer->m_edges.toggle();
    SPDLOG_DEBUG("Triangle edges toggled {}", viewer->m_edges.enabled() ? "on" : "off");
    interactor->GetRenderWindow()->Render();
}

void Viewer::on_start_render(vtkObject* caller, unsigned long, void* client_data, void*) {
    auto* renderer = static_cast<vtkRenderer*>(caller);
    auto* viewer = static_cast<Viewer*>(client_data);
    viewer->m_edges.apply_for_camera(renderer);
}

} // namespace vat::visualization
