#pragma once

/**
 * ASE GODOT ADAPTER - HUD HORIZON RIDGE
 *
 * @file        godot_hud_horizon_ridge.hpp
 * @brief       The two ridges of hills on the horizon, derived for the sky of the 3D scene
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, row N05 (m.html drawSky 422 to 424, design_tokens.json
 *              section VIVARIUM SKY RIDGES). The mockup closes its sky with two ridges, the far one first: each
 *              filled from sky_ridge_foot mockup units below the horizon up to sky_ridge_lift + height *
 *              (sky_ridge_base + sky_ridge_swing * sin(x * wave + phase)) * max(sky_ridge_relief_floor, relief)
 *              above it, relief the start relief of the landscape in metres (the start file's ground.height_scale_m;
 *              the debug layer's RELEVO reshapes the square under the camera and never the horizon). Its colour is the horizon's colour turned toward SKY_RIDGE_TONE by the ridge's
 *              share and dimmed to max(sky_ridge_dim_floor, light), light the mockup's light() as its one version
 *              SkyOvercast::land_light computes it (godot_hud_sky_overcast.hpp, m.html 411). The block derives these numbers and colours as a shade; the scene stands the ridges from it
 *              (light_plant_horizon_ridge, godot_plant_horizon_ridge.hpp). Nothing of the sky or the ground is
 *              changed here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_071
 * @category    render/pipeline/stage
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @register    carrier N0448
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_plant_horizon_ridge.hpp>  // HorizonRidgeShade, the shade the scene stands the ridges from

#include <godot_cpp/variant/color.hpp>

#include <string>

namespace ase::adp::godot {

/** What the horizon is this frame. */
struct HorizonRidgeFrame {
    ::godot::Color horizon;       // the sky's colour at the horizon as the sky shader draws it, the overcast grey mixed in
    double land_light = 1.0;      // the land's light of this frame, SkyOvercast::land_light (m.html light(), 411)
    double relief = 0.0;          // start relief of the landscape, metres (the start file's ground.height_scale_m)
    double frame_pixels = 0.0;    // screen pixels per mockup unit, the scale of the HUD's frame; 0 before the HUD stands
    double frame_origin_x = 0.0;  // screen x of the HUD frame's left edge, pixels
};

/** The ridges of hills on the horizon. */
class HorizonRidge {
public:
    /** Binds the measures and the colour; false with the missing name. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Writes the ridges of `now` into `out`; no frame pixels leave them unshown. */
    void shade(const HorizonRidgeFrame& now, HorizonRidgeShade& out) const;

private:
    double far_share_ = 0.0;      // sky_ridge_far_share
    double far_height_ = 0.0;     // sky_ridge_far_height
    double far_wave_ = 0.0;       // sky_ridge_far_wave
    double far_phase_ = 0.0;      // sky_ridge_far_phase
    double near_share_ = 0.0;     // sky_ridge_near_share
    double near_height_ = 0.0;    // sky_ridge_near_height
    double near_wave_ = 0.0;      // sky_ridge_near_wave
    double near_phase_ = 0.0;     // sky_ridge_near_phase
    double lift_ = 0.0;           // sky_ridge_lift
    double foot_ = 0.0;           // sky_ridge_foot
    double base_ = 0.0;           // sky_ridge_base
    double swing_ = 0.0;          // sky_ridge_swing
    double relief_floor_ = 0.0;   // sky_ridge_relief_floor
    double dim_floor_ = 0.0;      // sky_ridge_dim_floor
    ::godot::Color tone_;         // SKY_RIDGE_TONE
};

}  // namespace ase::adp::godot
