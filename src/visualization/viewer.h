#pragma once
#include <string>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <vtkActor.h>
#include <vtkAxesActor.h>
#include <vtkCallbackCommand.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPolyData.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTextActor.h>

#include "edge_overlay.h"

namespace vat::visualization {

/*
 * A window in the toolbox's house style: dark background, an orientation marker, the
 * nadir-pointing initial camera, a legend, and the triangle-edge overlay with its
 * keyboard toggle.
 *
 * A view builds one of these, adds what it wants to show, and calls Run(). Everything
 * that is the same between views lives here, so adding a fourth view means writing only
 * what makes it different.
 *
 * Non-copyable: the VTK observers hold a pointer to this object, so a copy would leave
 * them pointing at the original.
 */
class Viewer {
public:
    struct Config {
        /** Window title. */
        std::string title;

        /**
         * Order-independent transparency. Needed when the model itself is translucent,
         * because without it VTK blends actors in the order they were added rather than
         * by depth, putting far surfaces in front of near ones.
         */
        bool depth_peeling = false;

        /** Whether the triangle-edge overlay starts switched on. */
        bool show_triangle_edges = true;
    };

    explicit Viewer(Config config);

    Viewer(const Viewer&) = delete;
    Viewer& operator=(const Viewer&) = delete;

    /**
     * Adds a mesh in a single flat colour, with its triangle edges drawn over it.
     *
     * The edges go on the mesh's own actor rather than on a second wireframe one: VTK
     * offsets edge geometry in depth itself, so it sits on the surface instead of
     * z-fighting with it, which a separately overlaid actor would do.
     */
    void AddMesh(vtkPolyData* polydata, const glm::vec3& color, double opacity = 1.0);

    /**
     * Adds a mesh coloured from its own per-cell scalars.
     *
     * @param edge_fade_target - What the triangle edges fade into as they shrink. A
     *        scalar-coloured surface has no single colour of its own, so the caller has
     *        to name the closest stand-in.
     */
    void AddScalarColoredMesh(vtkPolyData* polydata, const glm::vec3& edge_fade_target);

    /** Adds anything that is not a mesh: arrows, hinge markers, and so on. */
    void AddActor(vtkSmartPointer<vtkActor> actor);

    /** Typical triangle size, which sets where the edge overlay starts to fade. */
    void SetMeanTriangleExtent(float extent__m);

    void AddBodyAxes(float bounding_sphere_radius);

    /**
     * Legend in the top-left corner: one row per entry, drawn in that entry's colour so
     * the colour itself is the key and no separate swatch is needed.
     */
    void AddLegend(const std::vector<std::string>& labels, const std::vector<glm::vec3>& colors);

    /** For the few things that add several actors of their own, such as the arc arrows. */
    vtkRenderer* renderer() { return m_renderer; }

    /** Shows the window and blocks until the user closes it. */
    void Run();

private:
    void InstallObservers();
    void AddEdgeHint();
    void PlaceCamera();

    // vtkCallbackCommand hands the client data back as void*; both of these take the
    // Viewer and forward to the members below.
    static void OnKeyPress(vtkObject* caller, unsigned long event_id, void* client_data, void* data);
    static void OnStartRender(vtkObject* caller, unsigned long event_id, void* client_data, void* data);

    Config m_config;

    vtkSmartPointer<vtkRenderer> m_renderer;
    vtkSmartPointer<vtkRenderWindow> m_render_window;
    vtkSmartPointer<vtkRenderWindowInteractor> m_interactor;
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> m_interaction_style;
    vtkSmartPointer<vtkAxesActor> m_axes;
    vtkSmartPointer<vtkOrientationMarkerWidget> m_orientation_widget;
    vtkSmartPointer<vtkTextActor> m_edge_hint;
    vtkSmartPointer<vtkCallbackCommand> m_key_callback;
    vtkSmartPointer<vtkCallbackCommand> m_render_callback;

    EdgeOverlay m_edges;
};

} // namespace vat::visualization
