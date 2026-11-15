#pragma once

/**
 * ASE GODOT ADAPTER - THE SHADERS OF THE PLANT AND ITS GROUND
 *
 * @file        godot_plant_look_shader.hpp
 * @brief       Files, uniform names and values of the ground, bark, organ and stand-in shaders of the care view
 * @description The four shaders godot_plant_look.cpp makes its materials of stand in adapter/ase-adp-godot/shaders/
 *              (godot_plant_shader.hpp): plant_stand_in, plant_organ and plant_bark take the swing of a branch
 *              from the include blocks sway_uniforms and sway_swing, plant_ground paints slope, ground state and
 *              grid. What a shader computes is described in its file. This header holds what the units that set
 *              the materials need: the file of each shader, the names of its uniforms and the values they are
 *              given. It is included by godot_plant_look.cpp, godot_plant_scene.cpp (the instance uniforms and the
 *              sway texture), godot_plant_environment.cpp (the exposure lift) and the tests of ground and bark -
 *              never by types.hpp, so a new uniform recompiles those units and no other.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-11
 * @modified    2026-11-14
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>

namespace ase::adp::godot {

// The files of the four shaders, below the stage's res://shaders/ (scripts/stage_native.py, --shader-dir). The
// three swinging ones set the swing of a branch in themselves: their head carries
// #include "res://shaders/sway_uniforms.gdshaderinc" and their vertex() #include "res://shaders/sway_swing.gdshaderinc",
// which ResourceLoader resolves when load_plant_shader loads the file. No unit splices shader code at run time.
constexpr const char* GODOT_STAND_IN_SHADER_FILE = "res://shaders/plant_stand_in.gdshader";  // a role whose file is missing, in its stand-in colour, swinging by its includes
constexpr const char* GODOT_ORGAN_SHADER_FILE = "res://shaders/plant_organ.gdshader";        // a bound organ template on a branch, swinging by its includes
constexpr const char* GODOT_BARK_SHADER_FILE = "res://shaders/plant_bark.gdshader";          // the continuous bark tube of every axis, white and stem-base mixed, swinging by its includes
constexpr const char* GODOT_BARK_SHOWN_ORDERS = "shown_orders";  // uniform of the bark shader: a ring of a higher order closes onto its axis (OrderBudget::shown)
constexpr const char* GODOT_BARK_COARSE = "coarse";              // uniform of the bark shader: true on the twig tube, where a ring up to the shown orders closes onto its axis instead
constexpr const char* GODOT_GROUND_SHADER_FILE = "res://shaders/plant_ground.gdshader";      // the ground of one soil

// The swing of a branch (usecase:101335 animate the branch sway): the two uniforms of sway_uniforms.gdshaderinc,
// set on every swinging material by fill_instances and fill_tube (godot_plant_scene_mesh.cpp).
constexpr const char* GODOT_SWAY_DATA = "sway_data";          // uniform of every swinging shader: the sway data texture of the instance node
constexpr const char* GODOT_SWAY_ROW = "sway_row";            // uniform of every swinging shader: GODOT_SWAY_ROW_INSTANCES
constexpr int32_t GODOT_ORGAN_SWAY_SURFACES = 1;              // surfaces of a bound template the organ shader can draw: the material is set for the whole mesh

// The instance uniforms the scene hands on every frame (mark_plant_stands, godot_plant_scene.cpp).
constexpr const char* GODOT_SHADER_SOIL_REL = "soil_rel";     // instance uniform of the ground material: soil water over field capacity of the focused patch, 0..1
constexpr const char* GODOT_SHADER_CONDITION = "condition";   // instance uniform of the leaf material: GODOT_COND_* of the plant's patch

// plant_stand_in.gdshader (operator order of 2026-10-23).
constexpr const char* GODOT_STAND_IN_COLOR = "stand_in_color";            // stand-in shader uniform: the colour of the name
constexpr const char* GODOT_STAND_IN_WILTED = "wilted_color";             // stand-in shader uniform: colour of a wilted stand
constexpr const char* GODOT_STAND_IN_DEAD = "dead_color";                 // stand-in shader uniform: colour of a dead stand
constexpr const char* GODOT_STAND_IN_WILTED_CODE = "wilted_code";         // stand-in shader uniform: GODOT_COND_WILTED
constexpr const char* GODOT_STAND_IN_DEAD_CODE = "dead_code";             // stand-in shader uniform: GODOT_COND_DEAD
constexpr const char* GODOT_STAND_IN_WET = "wet_shade";                   // stand-in shader uniform: GODOT_STAND_IN_WET_SHADE
constexpr const char* GODOT_STAND_IN_ROUGH = "stand_in_roughness";        // stand-in shader uniform: GODOT_STAND_IN_ROUGHNESS
constexpr float GODOT_STAND_IN_WET_SHADE = 0.55f;                         // share of its colour the ground keeps at field capacity: wet earth is darker
constexpr float GODOT_STAND_IN_ROUGHNESS = 0.9f;                          // roughness of every stand-in surface: matt, no texture to carry a sheen

// plant_organ.gdshader: what the BaseMaterial3D of a bound template carries.
constexpr const char* GODOT_ORGAN_COLOR = "organ_color";          // organ shader uniform: albedo colour of the bound material
constexpr const char* GODOT_ORGAN_ALBEDO = "organ_albedo";        // organ shader uniform: albedo texture
constexpr const char* GODOT_ORGAN_NORMAL = "organ_normal";        // organ shader uniform: normal texture
constexpr const char* GODOT_ORGAN_ROUGH = "organ_rough";          // organ shader uniform: roughness texture
constexpr const char* GODOT_ORGAN_LIGHT = "organ_light";          // organ shader uniform: backlight texture
constexpr const char* GODOT_ORGAN_ROUGHNESS = "organ_roughness";  // organ shader uniform: roughness of the bound material
constexpr const char* GODOT_ORGAN_UV = "organ_uv";                // organ shader uniform: UV scale of the bound material
constexpr const char* GODOT_ORGAN_CUT = "organ_cut";              // organ shader uniform: alpha scissor threshold, 0 for an opaque material

// The leaf in the colour of its season (m.html leafCol 486): the stand-in and the organ shader read the globals of
// godot_plant_leaf_season.hpp only where this uniform is 1, so the leaf takes them and the catkin, the fruit stand,
// the bark and the ground keep their own colour.
constexpr const char* GODOT_LEAF_SEASON = "leaf_season";          // stand-in and organ shader uniform: GODOT_LEAF_SEASON_ON on the leaf's material, 0 on every other
constexpr float GODOT_LEAF_SEASON_ON = 1.0f;                      // the value of GODOT_LEAF_SEASON on the leaf's material

// plant_bark.gdshader (usecase:100395 age the bark): the two bound bark materials.
constexpr const char* GODOT_BARK_WHITE_COLOR = "white_color";        // bark shader uniform: albedo colour of the white bark
constexpr const char* GODOT_BARK_WHITE_ALBEDO = "white_albedo";      // bark shader uniform: albedo texture of the white bark
constexpr const char* GODOT_BARK_WHITE_NORMAL = "white_normal";      // bark shader uniform: normal texture of the white bark
constexpr const char* GODOT_BARK_WHITE_ROUGH = "white_rough";        // bark shader uniform: roughness texture of the white bark
constexpr const char* GODOT_BARK_WHITE_ROUGHNESS = "white_roughness";  // bark shader uniform: roughness of the white bark
constexpr const char* GODOT_BARK_WHITE_UV = "white_uv";              // bark shader uniform: UV scale of the white bark
constexpr const char* GODOT_BARK_BASE_COLOR = "base_color";          // bark shader uniform: albedo colour of the stem-base bark
constexpr const char* GODOT_BARK_BASE_ALBEDO = "base_albedo";        // bark shader uniform: albedo texture of the stem-base bark
constexpr const char* GODOT_BARK_BASE_NORMAL = "base_normal";        // bark shader uniform: normal texture of the stem-base bark
constexpr const char* GODOT_BARK_BASE_ROUGH = "base_rough";          // bark shader uniform: roughness texture of the stem-base bark
constexpr const char* GODOT_BARK_BASE_ROUGHNESS = "base_roughness";  // bark shader uniform: roughness of the stem-base bark
constexpr const char* GODOT_BARK_BASE_UV = "base_uv";                // bark shader uniform: UV scale of the stem-base bark

// plant_ground.gdshader (usecase:100387 view the plant): slope, ground state, grid and the dither of the frame.
constexpr const char* GODOT_GROUND_COVER_COLOR = "cover_color";            // ground shader uniform: colour of the soil's cover (its role)
constexpr const char* GODOT_GROUND_COVER_ALBEDO = "cover_albedo";          // ground shader uniform: albedo texture of the cover, white while none is bound
constexpr const char* GODOT_GROUND_COVER_UV = "cover_uv";                  // ground shader uniform: UV scale of the cover
constexpr const char* GODOT_GROUND_SOIL = "soil_color";                    // ground shader uniform: bare soil of levelled ground and of a slope
constexpr const char* GODOT_GROUND_BANK = "bank_color";                    // ground shader uniform: an open bank
constexpr const char* GODOT_GROUND_BED = "bed_color";                      // ground shader uniform: the wet gravel of a brook's bed
constexpr const char* GODOT_GROUND_GRID = "grid_color";                    // ground shader uniform: colour of grid and contour lines
constexpr const char* GODOT_GROUND_SLOPE_BANK = "slope_bank";              // ground shader uniform: GODOT_GROUND_SLOPE_BANK_SHARE
constexpr const char* GODOT_GROUND_SLOPE_STEEP = "slope_steep";            // ground shader uniform: GODOT_GROUND_SLOPE_STEEP_SHARE
constexpr const char* GODOT_GROUND_GRID_STEP = "grid_step";                // ground shader uniform: distance of two height samples, metres (the ground's step_m)
constexpr const char* GODOT_GROUND_GRID_MAJOR_KEY = "grid_major";          // ground shader uniform: GODOT_GROUND_GRID_MAJOR
constexpr const char* GODOT_GROUND_CONTOUR = "contour_step";               // ground shader uniform: GODOT_GROUND_CONTOUR_M
constexpr const char* GODOT_GROUND_GRID_WEIGHT = "grid_weight";            // ground shader uniform: GODOT_GROUND_GRID_SHARE
constexpr const char* GODOT_GROUND_MAJOR_WEIGHT = "major_weight";          // ground shader uniform: GODOT_GROUND_MAJOR_SHARE
constexpr const char* GODOT_GROUND_CONTOUR_WEIGHT = "contour_weight";      // ground shader uniform: GODOT_GROUND_CONTOUR_SHARE
constexpr const char* GODOT_GROUND_WET = "wet_shade";                      // ground shader uniform: GODOT_STAND_IN_WET_SHADE
constexpr const char* GODOT_GROUND_ROUGH = "ground_roughness";             // ground shader uniform: roughness of the cover
constexpr const char* GODOT_GROUND_FRAME_STEP = "frame_step";              // ground shader uniform: GODOT_FRAME_STEP_LINEAR, the dither's full width
constexpr const char* GODOT_SHADER_GLOBAL_LIFT = "scene_lift";             // global shader parameter: the exposure lift of the frame (project.godot [shader_globals])
constexpr float GODOT_FRAME_STEP_LINEAR = 0.0019550342f;                   // one step of the Mobile frame in scene light: 2 / 1023 (A2B10G10R10_UNORM, luminance multiplier 2)
constexpr uint32_t GODOT_COLOR_GROUND_BANK = 0x7D6A52FFu;                  // an open bank: loam and gravel where the slope carries no grass
constexpr uint32_t GODOT_COLOR_GROUND_BED = 0x4B4C45FFu;                   // the bed of a brook: wet gravel
constexpr uint32_t GODOT_COLOR_GROUND_GRID = 0x10140CFFu;                  // grid and contour lines: near black, read on grass and on soil
constexpr float GODOT_GROUND_SLOPE_BANK_DEG = 18.0f;                       // slope above which the cover gives way to bare earth, degrees
constexpr float GODOT_GROUND_SLOPE_BANK_SHARE = 0.048943f;                 // 1 - cos(GODOT_GROUND_SLOPE_BANK_DEG): the shader compares 1 - normal.y with it
constexpr float GODOT_GROUND_SLOPE_STEEP_DEG = 35.0f;                      // slope above which the ground is an open bank, degrees
constexpr float GODOT_GROUND_SLOPE_STEEP_SHARE = 0.180848f;                // 1 - cos(GODOT_GROUND_SLOPE_STEEP_DEG)
constexpr float GODOT_GROUND_GRID_MAJOR = 8.0f;                            // cells between two strong grid lines: 1 m, one macro node of ase-terrain, at the micro sample step of 0.125 m (ground.step_m)
constexpr float GODOT_GROUND_CONTOUR_M = 0.1f;                             // height between two contour lines, metres: the 0.4 m bed shows four
constexpr float GODOT_GROUND_GRID_SHARE = 0.28f;                           // share of the line colour on a cell edge
constexpr float GODOT_GROUND_MAJOR_SHARE = 0.55f;                          // share of the line colour on a strong grid line
constexpr float GODOT_GROUND_CONTOUR_SHARE = 0.45f;                        // share of the line colour on a contour line

}  // namespace ase::adp::godot
