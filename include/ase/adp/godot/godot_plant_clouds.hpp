#pragma once

/**
 * ASE GODOT ADAPTER - THE CLOUD BANKS IN THE SKY OF THE CARE VIEW
 *
 * @file        godot_plant_clouds.hpp
 * @brief       The cloud banks of an overcast sky, stood in the sky of the care view's 3D scene
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, row N04. CloudBank (godot_hud_cloud_bank.hpp) lays
 *              the banks of the mockup's drawSky as ellipses in the units of the middle band's frame; the mockup
 *              draws them behind the hills and the tree, so they cannot lie on the HUD's canvas, which covers
 *              the crown. This unit stands each bank in the sky of the 3D scene instead: on the camera's ray
 *              through its point of the picture, at GODOT_SKY_CLOUD_DISTANCE, as a quad turned to the camera
 *              whose half axes span the bank's half axes in pixels there, times sky_cloud_reach. The ground,
 *              the hills and the tree stand nearer and cover it; the sun, the moon and the stars stand farther
 *              and are covered by it. Nothing of the weather is computed here: the banks come laid.
 *
 *              THE VEIL. A bank is no hard ellipse. GODOT_CLOUD_SHADER_FILE feathers its rim, frays it with a
 *              value noise and darkens its foot, every measure a token of SHA that CloudBank binds
 *              (CloudBankVeil); the bank's colour and opacity stay the instance's.
 *
 *              THE WIND. The banks run along the wind drift of the scene port, seen along the camera's right
 *              (cloud_drift_share); a snapshot without the wind record keeps the last drift (D2293).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_071
 * @category    render/pipeline/stage
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @register    N0449 (flora.weather.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_resource_manager.hpp>
#include <ase/adp/godot/godot_hud_cloud_bank.hpp>

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <vector>

namespace ase::adp::godot {

// The veil of a cloud bank (shaders/plant_clouds.gdshader, loaded by godot_plant_shader.hpp) and its uniforms,
// each filled from its token in CloudBankVeil. The veil is data: an edit of the file compiles no unit.
constexpr const char* GODOT_CLOUD_SHADER_FILE = "res://shaders/plant_clouds.gdshader";  // the veil of a cloud bank
constexpr const char* GODOT_CLOUD_REACH = "reach";      // uniform: quad half extent, half axes of the bank (sky_cloud_reach)
constexpr const char* GODOT_CLOUD_SOFT = "soft";        // uniform: feathered share of the half axis (sky_cloud_soft)
constexpr const char* GODOT_CLOUD_GRAIN = "grain";      // uniform: noise cells across the bank (sky_cloud_grain)
constexpr const char* GODOT_CLOUD_LIFT = "lift";        // uniform: noise reach into the rim, half axes (sky_cloud_lift)
constexpr const char* GODOT_CLOUD_SHADE = "shade";      // uniform: darkening at the foot of the bank (sky_cloud_shade)
constexpr double GODOT_SKY_CLOUD_COURSE = 1.0;          // pace before the first wind record: the mockup's drawSky runs the banks rightward at sky_cloud_speed

/**
 * Adds the cloud banks (GODOT_PLANT_CLOUDS) under the scene root: a multimesh of unit quads drawn with
 * GODOT_CLOUD_SHADER_FILE, which takes its colour and its opacity from each instance, casting no shadow. No
 * instance shows until light_plant_clouds stands banks. build_plant_sky calls it once.
 */
void build_plant_clouds(::godot::Node3D* scene);

/**
 * Stands `banks` (frame units) in the sky with `veil`: `frame` carries a frame point onto the canvas of the
 * viewport the camera draws into. No bank, no instance; no camera or no field, nothing. VivariumView::draw_hud
 * calls it every frame.
 */
void light_plant_clouds(::godot::Node3D* scene, const std::vector<CloudBankEllipse>& banks,
                        const CloudBankVeil& veil, const ::godot::Transform2D& frame);

/**
 * The pace the banks run with across the picture: the wind drift of the scene port (VivariumSkyView::drift_x,
 * drift_z, the wind share folded in, ase-pl-sky SKY_HOST_RECORD_WIND) along the camera's right on the world
 * grid (x east, z north), positive to the right, 0 in a calm. Before the port carried a wind record, and in a
 * scene without its camera, the banks keep the mockup's course, GODOT_SKY_CLOUD_COURSE. VivariumView::tick_hud
 * weighs each frame's seconds by it, so a turn of the wind or of the camera bends the run without a jump.
 */
double cloud_drift_share(::godot::Node3D* scene, const VivariumSkyView& sky);

}  // namespace ase::adp::godot
