#include "edge_overlay.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <vtkCamera.h>
#include <vtkProperty.h>

#include "edge_style.h"

namespace vat::visualization {

void EdgeOverlay::register_actor(vtkSmartPointer<vtkActor> actor, const glm::vec3& surface_color) {
    actor->GetProperty()->SetEdgeVisibility(1);
    actor->GetProperty()->SetEdgeColor(EDGE_COLOR.r, EDGE_COLOR.g, EDGE_COLOR.b);
    actor->GetProperty()->SetLineWidth(EDGE_LINE_WIDTH);

    m_meshes.push_back(Mesh{std::move(actor), surface_color});
}

float EdgeOverlay::triangle_pixel_extent(vtkRenderer* renderer, float extent__m) {
    if (renderer == nullptr) {
        return 0.0f;
    }
    vtkCamera* camera = renderer->GetActiveCamera();
    const int* viewport_size = renderer->GetSize();
    if (camera == nullptr || viewport_size == nullptr || viewport_size[1] <= 0) {
        return 0.0f;
    }

    // World-space height of the viewport at the focal plane.
    double world_height__m = 0.0;
    if (camera->GetParallelProjection()) {
        world_height__m = 2.0 * camera->GetParallelScale();
    } else {
        const double fov__rad = camera->GetViewAngle() * std::numbers::pi_v<double> / 180.0;
        world_height__m = 2.0 * camera->GetDistance() * std::tan(0.5 * fov__rad);
    }
    if (world_height__m <= 0.0) {
        return 0.0f;
    }

    return static_cast<float>(extent__m * viewport_size[1] / world_height__m);
}

float EdgeOverlay::fade_alpha(float extent__px) {
    const float t = std::clamp((extent__px - EDGE_FADE_MIN__PX) / (EDGE_FADE_FULL__PX - EDGE_FADE_MIN__PX), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t); // smoothstep
}

void EdgeOverlay::apply_for_camera(vtkRenderer* renderer) {
    if (m_meshes.empty()) {
        return;
    }

    const float alpha = m_enabled ? fade_alpha(triangle_pixel_extent(renderer, m_mean_triangle_extent__m)) : 0.0f;

    for (Mesh& mesh : m_meshes) {
        if (alpha <= EDGE_INVISIBLE_ALPHA) {
            mesh.actor->GetProperty()->SetEdgeVisibility(0);
            continue;
        }
        const glm::vec3 faded = mesh.surface_color + alpha * (EDGE_COLOR - mesh.surface_color);
        mesh.actor->GetProperty()->SetEdgeVisibility(1);
        mesh.actor->GetProperty()->SetEdgeColor(faded.r, faded.g, faded.b);
    }
}

} // namespace vat::visualization
