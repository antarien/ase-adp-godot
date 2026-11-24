#pragma once

/**
 * ASE GODOT ADAPTER - THE MESH OF AN ORGAN TEMPLATE
 *
 * @file        godot_plant_template_mesh.hpp
 * @brief       The arrays of one organ template and its tangents as an ArrayMesh
 * @description PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.6, the tangent per vertex: the organ templates are the
 *              only surfaces of the care view that carry a tangent, the one the bark shader turns with its
 *              knot (plant_bark.gdshader, TANGENT = knot_turn(CUSTOM2, TANGENT)) and the normal map of the bark
 *              is laid along. build_template (godot_plant_templates.hpp) hands the tangents beside the arrays;
 *              this one function puts them on ARRAY_TANGENT of the surface mesh_of builds from the same
 *              arrays (godot_plant_scene_mesh.cpp).
 *
 *              WHY A HEADER OF ITS OWN: mesh_of stands in godot_plant_scene.hpp and the mesh arrays in
 *              godot_plant_geometry.hpp, both read by most surfaces of the care view; a tangent there would
 *              compile all of them again for a field only the templates fill. Only the template mesh of the
 *              scene and the mesh source read this header.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101342 - hand organ shapes to app
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-23
 * @modified    2026-11-23
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_plant_geometry.hpp>

#include <godot_cpp/classes/array_mesh.hpp>

#include <vector>

namespace ase::adp::godot {

/**
 * The arrays of one organ template as an ArrayMesh, as mesh_of builds it, with `tangents` - four floats per
 * vertex of `arrays`, as build_template laid them - on ARRAY_TANGENT. An empty surface is no mesh; tangents
 * whose count is not four per vertex are a broken template set and give no mesh, as an empty surface does.
 */
::godot::Ref<::godot::ArrayMesh> template_mesh_of(const PlantMeshArrays& arrays, const std::vector<float>& tangents);

}  // namespace ase::adp::godot
