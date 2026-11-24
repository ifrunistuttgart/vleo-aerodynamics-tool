#pragma once
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include "Igeometry_shading_data.h"

/**
 * @file
 * @brief Actor factories shared by the views. Nothing here knows about windows or interaction:
 * each function turns geometry into something a renderer can show, so a new view can be
 * assembled out of these rather than repeating the VTK boilerplate.
 */
namespace vat::visualization::actors {

/**
 * @brief Arrow from the body-frame origin along dir, of the given length.
 *
 * The three radius/length arguments are fractions of the arrow's own length, which is
 * how vtkArrowSource parameterises itself -- so an arrow keeps its proportions at any
 * scale.
 *
 * @param dir Direction in the body frame; need not be normalised. A zero vector points along +x.
 * @param length Arrow length [m].
 * @param color RGB colour, components in [0, 1].
 * @param relative_shaft_radius Shaft radius as a fraction of length.
 * @param relative_tip_radius Tip radius as a fraction of length.
 * @param relative_tip_length Tip length as a fraction of length.
 * @return The arrow actor.
 */
vtkSmartPointer<vtkActor> Arrow(const glm::vec3& dir, float length, const glm::vec3& color,
                                double relative_shaft_radius, double relative_tip_radius, double relative_tip_length);

/**
 * @brief Arrow whose tail sits at tail__m rather than at the body-frame origin.
 * @param tail__m Tail position in the body frame [m].
 * @param dir Direction; see Arrow() for this and the remaining parameters.
 * @param length Arrow length [m].
 * @param color RGB colour, components in [0, 1].
 * @param relative_shaft_radius Shaft radius as a fraction of length.
 * @param relative_tip_radius Tip radius as a fraction of length.
 * @param relative_tip_length Tip length as a fraction of length.
 * @return The arrow actor.
 */
vtkSmartPointer<vtkActor> ArrowAt(const glm::vec3& tail__m, const glm::vec3& dir, float length, const glm::vec3& color,
                                  double relative_shaft_radius, double relative_tip_radius, double relative_tip_length);

/**
 * @brief Sphere actor.
 * @param center__m Centre in the body frame [m].
 * @param radius Radius [m].
 * @param color RGB colour, components in [0, 1].
 * @return The sphere actor.
 */
vtkSmartPointer<vtkActor> Sphere(const glm::vec3& center__m, float radius, const glm::vec3& color);

/**
 * @brief Curved arrow wrapping axis_hat, sweeping the way a positive angle turns the mesh.
 *
 * Two actors rather than one, so it adds itself to the renderer instead of returning.
 *
 * @param renderer Renderer to add the actors to.
 * @param center__m Centre of the arc in the body frame [m].
 * @param axis_hat Unit rotation axis.
 * @param radius Arc radius [m].
 * @param tube_radius Radius of the arc's tube [m].
 * @param color RGB colour, components in [0, 1].
 */
void AddArcArrow(vtkRenderer* renderer, const glm::vec3& center__m, const glm::vec3& axis_hat, float radius,
                 float tube_radius, const glm::vec3& color);

/**
 * @brief Adds the three body-frame axis arrows, scaled to the geometry.
 * @param renderer Renderer to add the arrows to.
 * @param bounding_sphere_radius Radius the arrows are scaled to [m]; 1.5 times it is the arrow
 *        length (1.5 if the radius is not positive).
 */
void AddBodyAxes(vtkRenderer* renderer, float bounding_sphere_radius);

/**
 * @brief One vtkPolyData per mesh, in mesh order, from the transformed vertices.
 *
 * The vertex array is not indexed -- it holds 9 floats per triangle -- so each mesh owns
 * a contiguous run of it, and the per-mesh triangle counts say where each run ends.
 *
 * @param geometry Geometry to convert, in its current pose.
 * @return One polydata per mesh.
 */
std::vector<vtkSmartPointer<vtkPolyData>> PerMeshPolyData(IGeometryShadingData& geometry);

/**
 * @brief The whole geometry as one vtkPolyData, with a per-triangle colour interpolated between
 * hidden_color and visible_color by that triangle's visibility.
 *
 * @param geometry Geometry to convert, in its current pose.
 * @param triangle_visibility Visibility in [0, 1] per triangle, in triangle order.
 * @param visible_color Colour at visibility 1.
 * @param hidden_color Colour at visibility 0.
 * @return The coloured polydata, colours as per-cell scalars.
 * @throws std::invalid_argument if triangle_visibility does not have one entry per triangle.
 */
vtkSmartPointer<vtkPolyData> VisibilityColoredPolyData(IGeometryShadingData& geometry,
                                                       const std::vector<float>& triangle_visibility,
                                                       const glm::vec3& visible_color, const glm::vec3& hidden_color);

/**
 * @brief Mesh actor in a single flat colour.
 * @param polydata Mesh to show.
 * @param color RGB colour, components in [0, 1].
 * @param opacity Opacity in [0, 1].
 * @return The actor.
 */
vtkSmartPointer<vtkActor> MeshActor(vtkPolyData* polydata, const glm::vec3& color, double opacity);

/**
 * @brief Mesh actor that takes its colours from the polydata's per-cell scalars.
 * @param polydata Mesh to show.
 * @return The actor.
 */
vtkSmartPointer<vtkActor> ScalarColoredMeshActor(vtkPolyData* polydata);

/**
 * @brief The side of a square with the mean triangle area -- a good enough stand-in for "how
 * big is a triangle" to drive the edge overlay's fade threshold.
 *
 * @param geometry Geometry to measure.
 * @return sqrt(mean triangle area) [m]; 0 for a geometry without triangles.
 */
float MeanTriangleExtent(IGeometryShadingData& geometry);

} // namespace vat::visualization::actors
