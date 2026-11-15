/**
 * ASE GODOT ADAPTER - HUD HORIZON RIDGE
 *
 * @file        godot_hud_horizon_ridge.cpp
 * @brief       The two ridges of hills on the horizon, derived for the sky of the 3D scene
 * @description See godot_hud_horizon_ridge.hpp.
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

#include <ase/adp/godot/godot_hud_horizon_ridge.hpp>

#include <ase/math/math.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool HorizonRidge::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"sky_ridge_far_share", &far_share_},
             {"sky_ridge_far_height", &far_height_},
             {"sky_ridge_far_wave", &far_wave_},
             {"sky_ridge_far_phase", &far_phase_},
             {"sky_ridge_near_share", &near_share_},
             {"sky_ridge_near_height", &near_height_},
             {"sky_ridge_near_wave", &near_wave_},
             {"sky_ridge_near_phase", &near_phase_},
             {"sky_ridge_lift", &lift_},
             {"sky_ridge_foot", &foot_},
             {"sky_ridge_base", &base_},
             {"sky_ridge_swing", &swing_},
             {"sky_ridge_relief_floor", &relief_floor_},
             {"sky_ridge_dim_floor", &dim_floor_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!theme.color("SKY_RIDGE_TONE", tone_)) {
        failed = "SKY_RIDGE_TONE";
        return false;
    }
    // a share turns the horizon toward the tone, from none to all of it
    if (!(far_share_ >= 0.0 && far_share_ <= 1.0) || !(near_share_ >= 0.0 && near_share_ <= 1.0)) {
        failed = "sky_ridge_far_share and sky_ridge_near_share must lie in 0..1";
        return false;
    }
    // the wave never dips below the horizon's lift: its lowest point base - swing stays at or above none
    if (!(far_height_ >= 0.0) || !(near_height_ >= 0.0) || !(foot_ >= 0.0) || !(swing_ >= 0.0) ||
        !(base_ - swing_ >= 0.0) || !(relief_floor_ > 0.0)) {
        failed = "sky_ridge_far_height, sky_ridge_near_height, sky_ridge_foot and sky_ridge_swing must not be negative, "
                 "sky_ridge_base not lie below sky_ridge_swing and sky_ridge_relief_floor be positive";
        return false;
    }
    if (!(dim_floor_ >= 0.0 && dim_floor_ <= 1.0)) {
        failed = "sky_ridge_dim_floor must lie in 0..1";
        return false;
    }
    return true;
}

void HorizonRidge::shade(const HorizonRidgeFrame& now, HorizonRidgeShade& out) const {
    out = HorizonRidgeShade{};
    if (!(now.frame_pixels > 0.0)) {
        return;
    }
    // the mockup's light() as SkyOvercast::land_light hands it, never below the ridges' dim floor
    const float dim = static_cast<float>(ase::math::max(dim_floor_, now.land_light));
    const ::godot::Color far = now.horizon.lerp(tone_, static_cast<float>(far_share_));
    const ::godot::Color near = now.horizon.lerp(tone_, static_cast<float>(near_share_));
    out.far_color = ::godot::Color(far.r * dim, far.g * dim, far.b * dim, 1.0f);
    out.near_color = ::godot::Color(near.r * dim, near.g * dim, near.b * dim, 1.0f);
    // the start relief of the landscape raises both ridges, never below its floor (m.html Math.max(0.4, S.relief));
    // the caller hands the start file's ground height scale, never the RELEVO debug chip that reshapes the square
    const double relief = ase::math::max(relief_floor_, now.relief);
    out.far_height = static_cast<float>(far_height_ * relief);
    out.near_height = static_cast<float>(near_height_ * relief);
    out.far_wave = static_cast<float>(far_wave_);
    out.far_phase = static_cast<float>(far_phase_);
    out.near_wave = static_cast<float>(near_wave_);
    out.near_phase = static_cast<float>(near_phase_);
    out.lift = static_cast<float>(lift_);
    out.foot = static_cast<float>(foot_);
    out.base = static_cast<float>(base_);
    out.swing = static_cast<float>(swing_);
    out.top = static_cast<float>(lift_ + ase::math::max(far_height_, near_height_) * relief * (base_ + swing_));
    out.frame_pixels = static_cast<float>(now.frame_pixels);
    out.frame_origin_x = static_cast<float>(now.frame_origin_x);
    out.shown = true;
}

}  // namespace ase::adp::godot
