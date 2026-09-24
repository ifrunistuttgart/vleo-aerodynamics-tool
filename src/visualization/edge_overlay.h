#pragma once
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

namespace vat::visualization {

/*
 * The triangle-edge overlay: the mesh actors it draws edges on, and the rule that
 * decides how much weight those edges carry at the current zoom.
 *
 * Which triangles a model is built from is not cosmetic. The shading pass rasterises
 * them one at a time, so an element too coarse to resolve a feature quietly biases the
 * area it reports; drawing the edges is how that gets noticed before it reaches a force.
 *
 * The overlay is drawn in screen space -- a line is one pixel wide however far away the
 * surface is -- while the triangles it outlines are in world space. Zooming out shrinks
 * the triangles but not the lines, so past a point the edges cover most of every
 * triangle and the model reads as flat grey instead of showing its shading. This class
 * measures how many pixels a typical triangle spans and fades the edges into the surface
 * colour before that happens.
 */
class EdgeOverlay {
public:
    /**
     * Adds a mesh actor to the overlay.
     *
     * @param actor - The mesh actor to draw triangle edges on.
     * @param surface_color - What the edges dissolve into as the triangles shrink.
     *        Fading towards the surface rather than towards nothing is what keeps the
     *        transition from popping.
     */
    void Register(vtkSmartPointer<vtkActor> actor, const glm::vec3& surface_color);

    /**
     * Sets the typical triangle size the fade is measured against, as the side of a
     * square with the mean triangle area. Costs one pass over the geometry's areas
     * rather than a measurement per frame.
     */
    void set_mean_triangle_extent__m(float extent__m) { m_mean_triangle_extent__m = extent__m; }

    void set_enabled(bool enabled) { m_enabled = enabled; }
    bool enabled() const { return m_enabled; }
    void Toggle() { m_enabled = !m_enabled; }

    /** True once at least one mesh has been registered. */
    bool empty() const { return m_meshes.empty(); }

    /**
     * Re-colours every registered mesh's edges for the camera as it stands. Called
     * before each render, so the fade tracks the zoom continuously.
     */
    void ApplyForCamera(vtkRenderer* renderer);

    /**
     * How many pixels a triangle of the given world-space extent spans, for this
     * renderer's camera. Handles both projection modes. Returns 0 if the camera or
     * viewport is unusable.
     */
    static float TrianglePixelExtent(vtkRenderer* renderer, float extent__m);

    /**
     * Weight the edges should carry at that pixel size, in [0, 1]. Smoothstepped, so
     * the fade has no hard corners.
     */
    static float FadeAlpha(float extent__px);

private:
    struct Mesh {
        vtkSmartPointer<vtkActor> actor;
        glm::vec3 surface_color;
    };

    std::vector<Mesh> m_meshes;
    bool m_enabled = true;
    float m_mean_triangle_extent__m = 0.0f;
};

} // namespace vat::visualization
