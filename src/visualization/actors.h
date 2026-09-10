#pragma once
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

#include "Igeometry_shading_data.h"

/*
 * Actor factories shared by the views. Nothing here knows about windows or interaction:
 * each function turns geometry into something a renderer can show, so a new view can be
 * assembled out of these rather than repeating the VTK boilerplate.
 */
namespace vat::visualization::actors {

/**
 * Arrow from the body-frame origin along dir, of the given length.
 *
 * The three radius/length arguments are fractions of the arrow's own length, which is
 * how vtkArrowSource parameterises itself -- so an arrow keeps its proportions at any
 * scale.
 */
vtkSmartPointer<vtkActor> Arrow(const glm::vec3& dir, float length, const glm::vec3& color,
    double relative_shaft_radius, double relative_tip_radius, double relative_tip_length);

/** Arrow whose tail sits at tail__m rather than at the body-frame origin. */
vtkSmartPointer<vtkActor> ArrowAt(const glm::vec3& tail__m, const glm::vec3& dir, float length,
    const glm::vec3& color, double relative_shaft_radius, double relative_tip_radius,
    double relative_tip_length);

vtkSmartPointer<vtkActor> Sphere(const glm::vec3& center__m, float radius, const glm::vec3& color);

/**
 * Curved arrow wrapping axis_hat, sweeping the way a positive angle turns the mesh.
 *
 * Two actors rather than one, so it adds itself to the renderer instead of returning.
 */
void AddArcArrow(vtkRenderer* renderer, const glm::vec3& center__m, const glm::vec3& axis_hat,
    float radius, float tube_radius, const glm::vec3& color);

/** The three body-frame axis arrows, scaled to the geometry. */
void AddBodyAxes(vtkRenderer* renderer, float bounding_sphere_radius);

/**
 * One vtkPolyData per mesh, in mesh order.
 *
 * The vertex array is not indexed -- it holds 9 floats per triangle -- so each mesh owns
 * a contiguous run of it, and the per-mesh triangle counts say where each run ends.
 */
std::vector<vtkSmartPointer<vtkPolyData>> PerMeshPolyData(IGeometryShadingData& geometry);

/**
 * The whole geometry as one vtkPolyData, with a per-triangle colour interpolated between
 * hidden_color and visible_color by that triangle's visibility.
 *
 * @throws std::invalid_argument if triangle_visibility does not have one entry per triangle.
 */
vtkSmartPointer<vtkPolyData> VisibilityColoredPolyData(IGeometryShadingData& geometry,
    const std::vector<float>& triangle_visibility,
    const glm::vec3& visible_color, const glm::vec3& hidden_color);

/** Mesh actor in a single flat colour. */
vtkSmartPointer<vtkActor> MeshActor(vtkPolyData* polydata, const glm::vec3& color, double opacity);

/** Mesh actor that takes its colours from the polydata's per-cell scalars. */
vtkSmartPointer<vtkActor> ScalarColoredMeshActor(vtkPolyData* polydata);

/**
 * The side of a square with the mean triangle area -- a good enough stand-in for "how
 * big is a triangle" to drive the edge overlay's fade threshold.
 */
float MeanTriangleExtent(IGeometryShadingData& geometry);

} // namespace vat::visualization::actors
