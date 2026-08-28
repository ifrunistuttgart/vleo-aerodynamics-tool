#pragma once
#include "Igeometry_shading_data.h"
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/vec3.hpp>

namespace vat {

/**
 * Definition of a single hinge: which mesh turns, and about what.
 *
 * These are exactly the arguments passed to
 * IGeometryManipulator::turn_mesh_around_axis(), collected so that a hinge can be
 * inspected visually before it is used to actually rotate anything.
 */
struct Hinge {
    /** Index of the mesh this hinge turns, in model-file order. */
    int mesh_id;
    /** The point the mesh turns about, in the body frame [m]. */
    glm::vec3 origin__m;
    /** The axis the mesh turns about. Need not be normalised. */
    glm::vec3 axis;
};

/**
 * Displays a geometry with its triangles colored according to their visibility to the
 * surrounding gas, based on the shading data and relative velocity.
 *
 * Blocks until the user closes the window.
 *
 * @param geometry - Reference to the geometry shading data.
 * @param triangle_visibility - Vector containing the visibility values for each triangle.
 * @param v_rel__m_per_s - relative velocity vector of the geometry with respect to the surrounding gas, in the satellite's body frame.
 */
void ShowShading(
    IGeometryShadingData& geometry,
    const std::vector<float>& triangle_visibility,
    const glm::vec3& v_rel__m_per_s
);

/**
 * Displays a geometry with each mesh in its own color, plus a legend naming them.
 *
 * Every legend entry is labelled "[<mesh_id>] <name>", where mesh_id is the value
 * turn_mesh_around_axis() expects and name comes from the model file. This is the view
 * to reach for when working out which mesh is which.
 *
 * Blocks until the user closes the window.
 *
 * @param geometry - Reference to the geometry shading data.
 */
void ShowMeshes(IGeometryShadingData& geometry);

/**
 * Displays the given hinges on top of the geometry, so hinge definitions can be checked
 * before they are used.
 *
 * Each hinge is drawn as a sphere at the hinge point, a straight arrow along the
 * rotation axis, and a curved arrow wrapping that axis in the direction a positive
 * angle__rad turns the mesh. Every mesh is drawn translucent, since a hinge usually sits
 * inside its own mesh and would otherwise be hidden by it. Each hinge is drawn in the
 * color of the mesh it turns, which is what identifies the two as belonging together.
 *
 * Blocks until the user closes the window.
 *
 * @param geometry - Reference to the geometry shading data.
 * @param hinges - The hinge definitions to display. Each mesh_id must be a valid mesh index.
 */
void ShowHinges(IGeometryShadingData& geometry, const std::vector<Hinge>& hinges);

} // namespace vat
