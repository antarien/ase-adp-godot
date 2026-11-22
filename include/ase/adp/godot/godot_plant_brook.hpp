#pragma once

/**
 * ASE GODOT ADAPTER - THE WATER OF THE CARE VIEW
 *
 * @file        godot_plant_brook.hpp
 * @brief       The node of the water in the care view's 3D scene and its fill
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 3, B0281: the water stands where ase-hydro puts it.
 *              build_water_table (godot_plant_water.hpp) lays the plane in the bed of every brook at its bed
 *              plus the depth ase-hydro gives it; the stream port (FloraHostStrmExptSystem) carries depth,
 *              velocity and course. In every pit it lays the open water hydrogeology.pit.v1 carries over the
 *              floor of its cell (D2506). This file hangs it into the scene
 *              in a ShaderMaterial of GODOT_WATER_SHADER_FILE. Nothing of the water is computed here.
 *              THE COLOUR OF THE WATER: the material sets the two colour uniforms of the shader to the
 *              green of a lowland brook (algae and fine silt), so the water reads green under a grey or a
 *              blue sky; the shallow colour lets the gravel of the bed show through, the deep one is the
 *              middle of the strip. They stand here and not in types.hpp, which nearly every unit of the
 *              adapter includes.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-11-01
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_pit_view.hpp>
#include <ase/adp/godot/godot_host_stream_view.hpp>
#include <ase/adp/godot/godot_plant_geometry.hpp>

#include <godot_cpp/classes/node3d.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

// The shader of the water (shaders/plant_water.gdshader, loaded by godot_plant_shader.hpp) and its uniforms.
constexpr const char* GODOT_WATER_SHADER_FILE = "res://shaders/plant_water.gdshader";  // the water of the brooks
constexpr const char* GODOT_WATER_VELOCITY_FULL = "velocity_full";  // uniform of GODOT_WATER_SHADER_FILE: GODOT_WATER_VELOCITY_FULL_M_S
constexpr const char* GODOT_WATER_SHALLOW = "shallow_color";  // uniform of GODOT_WATER_SHADER_FILE: the colour at no depth
constexpr const char* GODOT_WATER_DEEP = "deep_color";        // uniform of GODOT_WATER_SHADER_FILE: the colour at GODOT_WATER_DEPTH_FULL_M
constexpr float GODOT_WATER_SHALLOW_R = 0.24f;                // shallow brook water over gravel, red
constexpr float GODOT_WATER_SHALLOW_G = 0.40f;                // shallow brook water over gravel, green
constexpr float GODOT_WATER_SHALLOW_B = 0.24f;                // shallow brook water over gravel, blue
constexpr float GODOT_WATER_DEEP_R = 0.08f;                   // deep brook water, red
constexpr float GODOT_WATER_DEEP_G = 0.26f;                   // deep brook water, green
constexpr float GODOT_WATER_DEEP_B = 0.16f;                   // deep brook water, blue

// The groundwater standing in a pit (D2508): its own node and shader, still water without current or strokes.
// The node name and the colours stand here and not in scene/types.hpp, which nearly every unit of the adapter
// includes (a tier wipe).
constexpr const char* GODOT_PLANT_PIT_WATER = "PlantPitWater";  // the groundwater in the pits ase-hydro fills
constexpr const char* GODOT_PIT_WATER_SHADER_FILE = "res://shaders/pit_water.gdshader";  // the water of the pits
constexpr float GODOT_PIT_WATER_SHALLOW_R = 0.36f;            // shallow pit water over freshly dug loam, red
constexpr float GODOT_PIT_WATER_SHALLOW_G = 0.31f;            // shallow pit water over freshly dug loam, green
constexpr float GODOT_PIT_WATER_SHALLOW_B = 0.22f;            // shallow pit water over freshly dug loam, blue
constexpr float GODOT_PIT_WATER_DEEP_R = 0.12f;               // deep pit water, red
constexpr float GODOT_PIT_WATER_DEEP_G = 0.13f;               // deep pit water, green
constexpr float GODOT_PIT_WATER_DEEP_B = 0.10f;               // deep pit water, blue

/**
 * Hangs the empty water node (GODOT_PLANT_WATER) under the scene root, in a ShaderMaterial of
 * GODOT_WATER_SHADER_FILE with the green of GODOT_WATER_SHALLOW_* and GODOT_WATER_DEEP_*, and beside it the empty
 * pit water node (GODOT_PLANT_PIT_WATER) in a ShaderMaterial of GODOT_PIT_WATER_SHADER_FILE with the colours
 * GODOT_PIT_WATER_*, both casting no shadow. build_plant_scene calls it once, after the ground stands.
 */
void build_plant_water(::godot::Node3D* scene);

/**
 * Replaces the water with build_water_table over the stream records, the pit records and the ground mesh `ground` as
 * fill_plant_ground left it (PlantGroundSurface::arrays, a dig already written into it): the cells of a bed into
 * GODOT_PLANT_WATER, the cells only a pit floods into GODOT_PLANT_PIT_WATER (D2508); each node keeps its material.
 * A scene without the nodes draws no water, an empty ground no water either.
 *
 * @return the ground cells under water
 */
uint32_t fill_plant_water(::godot::Node3D* scene, const std::vector<VivariumStreamView>& streams,
                          const std::vector<VivariumPitView>& pits, const PlantMeshArrays& ground);

}  // namespace ase::adp::godot
