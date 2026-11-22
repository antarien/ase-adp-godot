#pragma once

/**
 * ASE GODOT ADAPTER - THE SHIMMER OF HOT AIR IN THE CARE VIEW
 *
 * @file        godot_plant_haze.hpp
 * @brief       The shimmer of hot air over the care view's 3D scene
 * @description DSGN_130 130/FOGWR: from 313 K the rising warm air breaks the light unevenly, the horizon
 *              begins to waver, and up to the heat damage limit of 323 K the shimmer grows to full strength;
 *              whoever watches a shimmering plain from afar makes out shapes in it only blurred.
 *              ase-atmosphere computes the strength from the temperature (AtmoSimTmpHazeSystem,
 *              ATM_VIS_HEAT_HAZE), ase-pl-sky hands it on as the shimmer of the fog record
 *              (SkyHostScneExptSystem). The view shows it: a quad in front of the camera reads the drawn
 *              image and shifts its rows sideways, the more the farther away what a row shows lies, between
 *              where the fog of the same record begins and where it is full. Nothing of the shimmer is
 *              computed here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_130
 * @category    render/pipeline/stage
 * @usecase     usecase:101515 - veil sight with weather
 * @actor       Weather Veil Controller (WEATVEICON)
 * @created     2026-10-28
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_sky_view.hpp>

#include <godot_cpp/classes/node3d.hpp>

namespace ase::adp::godot {

// The shader of the shimmer (shaders/plant_haze.gdshader, loaded by godot_plant_shader.hpp), its uniforms and the
// values of its wave. Its uniforms stand here and not in types.hpp: a new one recompiles this unit's users only.
constexpr const char* GODOT_HAZE_SHADER_FILE = "res://shaders/plant_haze.gdshader";  // the shimmer of hot air
constexpr const char* GODOT_HAZE_SHIMMER = "shimmer";        // haze shader uniform: shimmer of the fog record, 0..1
constexpr const char* GODOT_HAZE_NEAR = "near_m";            // haze shader uniform: distance where the fog begins, metres - the shimmer starts there
constexpr const char* GODOT_HAZE_FAR = "far_m";              // haze shader uniform: distance where the fog is full, metres - the shimmer is full there
constexpr const char* GODOT_HAZE_ROWS = "wave_rows";         // haze shader uniform: phase of the wave over the screen height, radians
constexpr const char* GODOT_HAZE_SPEED = "wave_speed";       // haze shader uniform: phase the wave runs per second, radians
constexpr const char* GODOT_HAZE_REACH = "wave_reach";       // haze shader uniform: sideways shift at full shimmer, share of the screen width
constexpr const char* GODOT_HAZE_DRIFT = "drift";            // haze shader uniform: the wind drift along the camera's right, -1..1, the wind share folded in: the rows lean and run downwind
constexpr float GODOT_HAZE_WAVE_ROWS = 160.0f;               // radians over the screen height: about 25 rows of rising air across the view
constexpr float GODOT_HAZE_WAVE_SPEED = 6.0f;                // radians per second: a row wavers about once a second
constexpr float GODOT_HAZE_WAVE_REACH = 0.003f;              // share of the screen width at full shimmer: a few pixels, shapes blur, the view stays
constexpr float GODOT_HAZE_QUAD_SIZE = 2.0f;                 // side of the quad: it spans clip space from -1 to 1
constexpr float GODOT_HAZE_QUAD_DEPTH = 1.0f;                // metres in front of the camera the quad hangs, inside every frustum

/**
 * Hangs the hidden haze quad (GODOT_PLANT_HAZE) under the camera of the scene (GODOT_PLANT_CAMERA),
 * GODOT_HAZE_QUAD_DEPTH in front of it, in a ShaderMaterial of GODOT_HAZE_SHADER_FILE whose wave stands
 * at GODOT_HAZE_WAVE_ROWS, GODOT_HAZE_WAVE_SPEED and GODOT_HAZE_WAVE_REACH. A scene without its camera
 * gets none. build_plant_scene calls it once, after the camera stands.
 */
void build_plant_haze(::godot::Node3D* scene);

/**
 * Shows the haze quad while the fog record carries a shimmer above 0 and hands it the shimmer and the near
 * and far distance of the fog; at 0 the quad is hidden and the image goes unshifted. A sky the port did not
 * carry yet changes nothing. The wind drift of the scene port (VivariumSkyView::drift_x, drift_z) goes in
 * along the camera's right (GODOT_HAZE_DRIFT), so the rising rows lean and run downwind across the view;
 * before the first wind record the drift is 0 and the rows rise straight (D2293).
 */
void veil_plant_scene(::godot::Node3D* scene, const VivariumSkyView& sky);

}  // namespace ase::adp::godot
