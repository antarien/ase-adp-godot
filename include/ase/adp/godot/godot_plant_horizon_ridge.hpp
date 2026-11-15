#pragma once

/**
 * ASE GODOT ADAPTER - THE RIDGES OF HILLS ON THE HORIZON OF THE CARE VIEW
 *
 * @file        godot_plant_horizon_ridge.hpp
 * @brief       The two ridges of hills the mockup draws on the horizon, stood in the sky of the care view's 3D scene
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, row N05. The mockup's drawSky closes with two ridges of
 *              hills (m.html 422 to 424): the far one, then the near one, each filled from below the horizon up to a
 *              wavy edge whose height follows the start relief of the landscape, in the horizon's colour turned toward a dark
 *              green and dimmed with the light. HorizonRidge (godot_hud_horizon_ridge.hpp) derives the numbers and the
 *              colours as the shade; this unit stands the ridges in the 3D scene: one upright square ahead of the
 *              camera, its middle on the horizon, nearer than the cloud banks (GODOT_SKY_CLOUD_DISTANCE) and the sky
 *              discs, so the ridges cover sun, halo, stars and banks as the canvas draws them last, and farther than
 *              the ground, which covers their foot. Its heights are mockup units, so its size follows the HUD frame's
 *              pixels per unit and the camera's view, the way the halo is sized (godot_plant_sun_halo.hpp); the edge
 *              itself is drawn by GODOT_RIDGE_SHADER_FILE across the frame. Nothing of the sky or the ground is
 *              computed here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_071
 * @category    render/pipeline/stage
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace ase::adp::godot {

// The ridges stand on one square nearer than the cloud banks, so they cover the banks and the discs behind them. It
// stands here and not in types.hpp, which nearly every unit of the adapter includes.
constexpr const char* GODOT_PLANT_HORIZON_RIDGE = "PlantHorizonRidge";  // the ridges of hills, one upright square ahead of the camera
constexpr float GODOT_RIDGE_DISTANCE = 800.0f;     // metres from the camera to the square along the ground: nearer than the cloud banks (900)
constexpr float GODOT_RIDGE_SPAN = 1.5f;           // width of the square as a share of the screen's width at its depth: the frame's sides stay covered under any turn
constexpr const char* GODOT_RIDGE_SHADER_FILE = "res://shaders/plant_horizon_ridge.gdshader";  // the look of the ridges, data: an edit compiles no unit
constexpr const char* GODOT_RIDGE_FAR_COLOR = "ridge_far_color";    // uniform: colour of the far ridge
constexpr const char* GODOT_RIDGE_NEAR_COLOR = "ridge_near_color";  // uniform: colour of the near ridge
constexpr const char* GODOT_RIDGE_FAR = "ridge_far";                // uniform: height, wave and phase of the far ridge
constexpr const char* GODOT_RIDGE_NEAR = "ridge_near";              // uniform: height, wave and phase of the near ridge
constexpr const char* GODOT_RIDGE_LIFT = "ridge_lift";              // uniform: units the foot of the wave stands above the horizon
constexpr const char* GODOT_RIDGE_BASE = "ridge_base";              // uniform: share of the height the wave never falls below
constexpr const char* GODOT_RIDGE_SWING = "ridge_swing";            // uniform: share of the height the wave swings by
constexpr const char* GODOT_RIDGE_TOP = "ridge_top";                // uniform: units above the horizon at the square's top edge
constexpr const char* GODOT_RIDGE_FOOT = "ridge_foot";              // uniform: units below the horizon at the square's bottom edge
constexpr const char* GODOT_RIDGE_FRAME_X = "frame_origin_x";       // uniform: screen x of the frame's left edge, pixels
constexpr const char* GODOT_RIDGE_FRAME_PIXELS = "frame_pixels";    // uniform: screen pixels per mockup unit

/**
 * The ridges as HorizonRidge::shade derives them for this frame (m.html drawSky 422 to 424). Heights are mockup
 * units above the horizon, already scaled by the relief; waves are radians per unit across the frame. The defaults
 * draw nothing.
 */
struct HorizonRidgeShade {
    bool shown = false;           // false before the HUD stands (no frame pixels): no ridges
    ::godot::Color far_color;     // the far ridge: the horizon toward SKY_RIDGE_TONE by sky_ridge_far_share, dimmed
    ::godot::Color near_color;    // the near ridge: the horizon toward SKY_RIDGE_TONE by sky_ridge_near_share, dimmed
    float far_height = 0.0f;      // sky_ridge_far_height times max(sky_ridge_relief_floor, relief)
    float far_wave = 0.0f;        // sky_ridge_far_wave
    float far_phase = 0.0f;       // sky_ridge_far_phase
    float near_height = 0.0f;     // sky_ridge_near_height times max(sky_ridge_relief_floor, relief)
    float near_wave = 0.0f;       // sky_ridge_near_wave
    float near_phase = 0.0f;      // sky_ridge_near_phase
    float lift = 0.0f;            // sky_ridge_lift
    float foot = 0.0f;            // sky_ridge_foot
    float base = 0.0f;            // sky_ridge_base
    float swing = 0.0f;           // sky_ridge_swing
    float top = 0.0f;             // the highest a ridge can reach: lift plus the larger height times (base + swing)
    float frame_pixels = 0.0f;    // screen pixels per mockup unit, the scale of the HUD's frame
    float frame_origin_x = 0.0f;  // screen x of the HUD frame's left edge, pixels
};

/**
 * Adds the ridges (GODOT_PLANT_HORIZON_RIDGE) under the scene root: a square of one metre in the material of
 * GODOT_RIDGE_SHADER_FILE that casts no shadow, hidden until light_plant_horizon_ridge stands it.
 * build_plant_sky calls it once.
 */
void build_plant_horizon_ridge(::godot::Node3D* scene);

/**
 * Stands the ridges GODOT_RIDGE_DISTANCE metres ahead of the camera along the ground, upright, its sides along the
 * camera's right and the world's up, its horizon line on the ray from the camera through `edge`, the square's far
 * edge straight ahead (plant_ground_far_edge, godot_plant_ground_season.hpp): the mockup's ridges stand on its HOR,
 * where its ground begins (m.html 358 and 424), and the ground of the 3D scene ends at that edge, so the ridges
 * stand on the ground as the canvas draws them, without a band of sky below their foot. The square reaches from the
 * shade's foot units below that line to its top units above, GODOT_RIDGE_SPAN screens wide. A unit is the shade's
 * frame_pixels on the screen, and a pixel at the square's depth along the view spans 2·depth·tan(fov/2) / viewport
 * height metres. Hidden while the shade is not shown, without an edge (nullptr), for an edge not ahead of the camera
 * or a camera looking straight down. The view calls it every frame after light_plant_scene, with the shade
 * VivariumView binds from the HUD's theme.
 */
void light_plant_horizon_ridge(::godot::Node3D* scene, const HorizonRidgeShade& ridge, const ::godot::Vector3* edge);

}  // namespace ase::adp::godot
