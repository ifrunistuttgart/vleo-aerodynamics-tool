#include "edge_overlay.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <vtkCamera.h>
#include <vtkProperty.h>

#include "edge_style.h"

namespace vat::visualization {

void EdgeOverlay::Register(vtkSmartPointer<vtkActor> actor, const glm::vec3& surface_color) {
    actor->GetProperty()->SetEdgeVisibility(1);
    actor->GetProperty()->SetEdgeColor(kEdgeColor.r, kEdgeColor.g, kEdgeColor.b);
    actor->GetProperty()->SetLineWidth(kEdgeLineWidth);

    m_meshes.push_back(Mesh{std::move(actor), surface_color});
}

float EdgeOverlay::TrianglePixelExtent(vtkRenderer* renderer, float extent__m) {
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

float EdgeOverlay::FadeAlpha(float extent__px) {
    const float t = std::clamp(
        (extent__px - kEdgeFadeMinPx) / (kEdgeFadeFullPx - kEdgeFadeMinPx), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t); // smoothstep
}

void EdgeOverlay::ApplyForCamera(vtkRenderer* renderer) {
    if (m_meshes.empty()) {
        return;
    }

    const float alpha = m_enabled
        ? FadeAlpha(TrianglePixelExtent(renderer, m_mean_triangle_extent__m))
        : 0.0f;

    for (Mesh& mesh : m_meshes) {
        if (alpha <= kEdgeInvisibleAlpha) {
            mesh.actor->GetProperty()->SetEdgeVisibility(0);
            continue;
        }
        const glm::vec3 faded = mesh.surface_color + alpha * (kEdgeColor - mesh.surface_color);
        mesh.actor->GetProperty()->SetEdgeVisibility(1);
        mesh.actor->GetProperty()->SetEdgeColor(faded.r, faded.g, faded.b);
    }
}

} // namespace vat::visualization
