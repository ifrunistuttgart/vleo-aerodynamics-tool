#include "show_mesh.h"

#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/geometric.hpp>

#include <vtkAutoInit.h>

#include "actors.h"
#include "palette.h"
#include "viewer.h"

VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

namespace vat::visualization {
namespace {

// Opacity of the meshes in ShowHinges(). Every mesh is translucent there, including the
// ones a hinge belongs to: a hinge normally sits inside its own mesh, so drawing that
// mesh opaque would hide the very markers the view exists to show. Each hinge is drawn
// in the colour of the mesh it turns, which is what ties the two together instead.
constexpr double kHingeViewMeshOpacity = 0.5;

/*
 * 'id <mesh_id>  "<name>"' for every mesh. mesh_id is both the index into the mesh
 * arrays and the argument turn_mesh_around_axis() takes, so the legend doubles as a
 * lookup table for rotating a mesh.
 *
 * The id is spelled out and the name quoted because a model may well name its meshes
 * with bare numbers, and "[4] 3" gives the reader no way to tell which number is which.
 * 'id 4  "3"' does.
 */
std::vector<std::string> MeshLabels(IGeometryShadingData& geometry) {
    const std::span<const std::string> names = geometry.get_mesh_names();
    std::vector<std::string> labels;
    labels.reserve(names.size());
    for (std::size_t i = 0; i < names.size(); ++i) {
        labels.push_back(std::format("id {}  \"{}\"", i, names[i]));
    }
    return labels;
}

void RequireMeshes(IGeometryShadingData& geometry, const char* view) {
    if (geometry.get_num_triangles_per_mesh().empty()) {
        SPDLOG_ERROR("{}: geometry contains no meshes", view);
        throw std::invalid_argument("geometry contains no meshes");
    }
}

/*
 * Validates hinges up front: a bad mesh_id here almost certainly means the same bad id
 * would have been handed to turn_mesh_around_axis(), which is the mistake this view
 * exists to catch, so say so rather than silently drawing a hinge attached to nothing.
 */
void ValidateHinges(const std::vector<Hinge>& hinges, std::size_t num_meshes) {
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
}

} // namespace

void ShowShading(
    IGeometryShadingData& geometry,
    const std::vector<float>& triangle_visibility,
    const glm::vec3& v_rel__m_per_s,
    const ViewOptions& options) {
    SPDLOG_DEBUG("ShowShading start (triangles={}, |v_rel|={})",
        geometry.get_num_triangles(), glm::length(v_rel__m_per_s));

    // Throws if the visibility vector does not match the geometry.
    vtkSmartPointer<vtkPolyData> mesh = actors::VisibilityColoredPolyData(
        geometry, triangle_visibility, kVisibleTriangleColor, kHiddenTriangleColor);

    const float bounding_sphere_radius = geometry.get_bounding_sphere_radius();
    const float wind_length =
        bounding_sphere_radius > 0.0f ? bounding_sphere_radius * 1.3f : 1.0f;
    const glm::vec3 wind_hat = glm::length(v_rel__m_per_s) > 0.0f
        ? glm::normalize(v_rel__m_per_s)
        : glm::vec3(1.0f, 0.0f, 0.0f);

    Viewer viewer(Viewer::Config{
        .title = "VAT Mesh Visibility + Wind",
        .depth_peeling = false,
        .show_triangle_edges = options.show_triangle_edges,
    });
    viewer.SetMeanTriangleExtent(actors::MeanTriangleExtent(geometry));

    // This surface is coloured per triangle rather than per mesh, so the edges fade into
    // the midpoint of the two visibility colours -- the best single stand-in there is.
    viewer.AddScalarColoredMesh(mesh, 0.5f * (kVisibleTriangleColor + kHiddenTriangleColor));
    viewer.AddActor(actors::Arrow(wind_hat, wind_length * 1.2f, kWindColor, 0.0045, 0.012, 0.030));
    viewer.AddBodyAxes(bounding_sphere_radius);

    viewer.Run();
}

void ShowMeshes(IGeometryShadingData& geometry, const ViewOptions& options) {
    RequireMeshes(geometry, "ShowMeshes");

    const std::vector<std::string> labels = MeshLabels(geometry);
    SPDLOG_DEBUG("ShowMeshes start (meshes={}, triangles={})",
        labels.size(), geometry.get_num_triangles());

    const std::vector<vtkSmartPointer<vtkPolyData>> per_mesh = actors::PerMeshPolyData(geometry);

    Viewer viewer(Viewer::Config{
        .title = "VAT Meshes",
        .depth_peeling = false,
        .show_triangle_edges = options.show_triangle_edges,
    });
    viewer.SetMeanTriangleExtent(actors::MeanTriangleExtent(geometry));

    std::vector<glm::vec3> colors;
    colors.reserve(per_mesh.size());
    for (std::size_t i = 0; i < per_mesh.size(); ++i) {
        colors.push_back(MeshColor(i));
        viewer.AddMesh(per_mesh[i], colors.back());
    }

    viewer.AddBodyAxes(geometry.get_bounding_sphere_radius());
    viewer.AddLegend(labels, colors);

    viewer.Run();
}

void ShowHinges(IGeometryShadingData& geometry, const std::vector<Hinge>& hinges,
    const ViewOptions& options) {
    RequireMeshes(geometry, "ShowHinges");

    const std::vector<std::string> mesh_labels = MeshLabels(geometry);
    SPDLOG_DEBUG("ShowHinges start (meshes={}, hinges={})", mesh_labels.size(), hinges.size());
    ValidateHinges(hinges, mesh_labels.size());

    const std::vector<vtkSmartPointer<vtkPolyData>> per_mesh = actors::PerMeshPolyData(geometry);

    Viewer viewer(Viewer::Config{
        .title = "VAT Hinges",
        .depth_peeling = true, // the whole model is translucent here
        .show_triangle_edges = options.show_triangle_edges,
    });
    viewer.SetMeanTriangleExtent(actors::MeanTriangleExtent(geometry));

    for (std::size_t i = 0; i < per_mesh.size(); ++i) {
        viewer.AddMesh(per_mesh[i], MeshColor(i), kHingeViewMeshOpacity);
    }

    const float bounding_sphere_radius = geometry.get_bounding_sphere_radius();
    const float scale = bounding_sphere_radius > 0.0f ? bounding_sphere_radius : 1.0f;
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

        viewer.AddActor(actors::Sphere(hinge.origin__m, marker_radius, color));
        viewer.AddActor(actors::ArrowAt(tail, axis_hat, axis_length, color, 0.004, 0.011, 0.028));
        actors::AddArcArrow(viewer.renderer(), hinge.origin__m, axis_hat, arc_radius,
            arc_tube_radius, color);
    }

    viewer.AddBodyAxes(bounding_sphere_radius);

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
        viewer.AddLegend(legend_labels, legend_colors);
    }

    viewer.Run();
}

} // namespace vat::visualization
