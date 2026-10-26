#pragma once

/**
 * ASE GODOT ADAPTER - ORGAN TEMPLATES OF THE PLANTS
 *
 * @file        godot_plant_templates.hpp
 * @brief       Builds one organ template from the corner and triangle records of the template ports
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2: the fine and the coarse cylinder and the unit
 *              leaf are built by ase-render-data on the unit (usecase:101011 tessellate the organ templates);
 *              the view takes them as the template ports carried them. Nothing is computed: no ring, no normal,
 *              no winding.
 *
 *              THE WINDING IS THE PORT'S: Godot takes the CLOCKWISE triangle of a mesh, seen from outside, as
 *              its front face, and ase-render-data lays every template triangle in that order
 *              (RenderDataTmplCylCalcSystem, RenderDataTmplLeafCalcSystem). The corners go to the index buffer
 *              in the order the triangle record names them.
 *
 *              NO GODOT TYPE: plain numbers in, plain arrays out, so the host tests check it without a Godot
 *              process.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101342 - hand organ shapes to app
 * @actor       Application Layer Perception Control (ALPCTL)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/**
 * Appends one organ template to `out`, taken as the template ports carried it: one vertex per corner record
 * of `code` - its place on the unit, its normal and its texture coordinates - and per triangle record of
 * `code` the triangle over the corners its three indices name, in the order the record gives them.
 * GODOT_TMPL_FINE is the cylinder of the internodes up to the shown orders, GODOT_TMPL_CORS the one above
 * them, and the fine one stands in for a catkin or a fruit stand whose template asset is missing;
 * GODOT_TMPL_LEAF stands in for a leaf or a seed. A triangle whose corner the port did not carry is not drawn;
 * an index named twice is drawn once.
 *
 * @return the triangles drawn
 */
uint32_t build_template(const std::vector<VivariumTemplateCornerView>& corners,
                        const std::vector<VivariumTemplateTriangleView>& triangles, uint32_t code,
                        PlantMeshArrays& out);

}  // namespace ase::adp::godot
