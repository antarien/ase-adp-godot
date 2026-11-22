/**
 * ASE GODOT ADAPTER - HUD GROUND SEASON
 *
 * @file        godot_hud_ground_season.cpp
 * @brief       The colour of the ground by season, snow, depth and horizon, derived for the 3D scene
 * @description See godot_hud_ground_season.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0460 (flora.day.v1: the season), N0449 (sky.weather.v1: the snow), carrier N0448
 * @created     2026-11-12
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ground_season.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <cstring>
#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool GroundSeason::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"ground_snow_share", &snow_share_},
             {"ground_depth_floor", &depth_floor_},
             {"ground_depth_span", &depth_span_},
             {"ground_horizon_share", &horizon_share_},
             {"ground_horizon_dim", &horizon_dim_},
             {"ground_row_power", &row_power_},
             {"hud_frame_height", &frame_height_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    // every share turns one colour toward another, from none to all of it
    for (const std::pair<const char*, double>& share : std::initializer_list<std::pair<const char*, double>>{
             {"ground_snow_share", snow_share_},
             {"ground_horizon_share", horizon_share_},
             {"ground_horizon_dim", horizon_dim_}}) {
        if (!(share.second >= 0.0 && share.second <= 1.0)) {
            failed = std::string(share.first) + " must lie in 0..1";
            return false;
        }
    }
    // the near edge never stands brighter than the cover itself: floor and floor + span lie in 0..1
    if (!(depth_floor_ >= 0.0 && depth_span_ >= 0.0 && depth_floor_ + depth_span_ <= 1.0)) {
        failed = "ground_depth_floor and ground_depth_span must not be negative and add up to at most 1";
        return false;
    }
    // rowY lays the rows from the horizon down to the frame's foot: a positive power over a frame with height
    if (!(row_power_ > 0.0 && frame_height_ > 0.0)) {
        failed = "ground_row_power and hud_frame_height must be positive";
        return false;
    }
    seasons_.clear();
    for (const char* name : {"GROUND_SEASON_GLACIX", "GROUND_SEASON_SALTATRIX", "GROUND_SEASON_IGNIX",
                             "GROUND_SEASON_HALITRIX"}) {
        ::godot::Color colour;
        if (!theme.color(name, colour)) {
            failed = name;
            return false;
        }
        seasons_.push_back(colour);
    }
    if (!theme.color("GROUND_SNOW", snow_)) {
        failed = "GROUND_SNOW";
        return false;
    }
    return true;
}

bool GroundSeason::shade(const GroundSeasonFrame& now, GroundSeasonShade& out, std::string& failed) const {
    out = GroundSeasonShade{};
    if (now.season < 0 || static_cast<size_t>(now.season) >= seasons_.size()) {
        failed = "GroundSeason: the season " + std::to_string(now.season) + " has no colour in GROUND_SEASON_*";
        return false;
    }
    if (!(now.frame_pixels > 0.0)) {
        failed = "GroundSeason: the frame carries no pixels per mockup unit";
        return false;
    }
    // THE GROUND IS THE WORLD'S UNDER AN OPEN SKY: its cover, the bare soil of a levelled disc and the bed stand as
    // the soils paint them. The season's colour, the darkened rows and the haze toward the horizon lie over it only
    // in murky weather, the mockup's overcast(): rain, drizzle, snow, fog and a clouded sky. No kind, no haze
    bool murky = false;
    for (const char* kind : {GODOT_HUD_WEATHER_RAIN, GODOT_HUD_WEATHER_DRIZZLE, GODOT_HUD_WEATHER_SNOW,
                             GODOT_HUD_WEATHER_FOG, GODOT_HUD_WEATHER_CLOUDY}) {
        murky = murky || (now.weather != nullptr && std::strcmp(now.weather, kind) == 0);
    }
    if (!murky) {
        return true;
    }
    // the season's colour, turned toward the snow while it snows (m.html: S.weather==='snow')
    ::godot::Color season = seasons_[static_cast<size_t>(now.season)];
    if (now.weather != nullptr && std::strcmp(now.weather, GODOT_HUD_WEATHER_SNOW) == 0) {
        season = season.lerp(snow_, static_cast<float>(snow_share_));
        // m.html groundCol whitens every row of the ground, so the same snow lies on the bare soil of a levelled disc
        // and on the banks; the bed of the brook stays under its water
        out.snow = ::godot::Color(snow_.r, snow_.g, snow_.b, static_cast<float>(snow_share_));
    }
    const auto dim = static_cast<float>(horizon_dim_);
    out.season = ::godot::Color(season.r, season.g, season.b, 1.0f);
    out.depth_floor = static_cast<float>(depth_floor_);
    out.depth_span = static_cast<float>(depth_span_);
    out.horizon = ::godot::Color(now.horizon.r * dim, now.horizon.g * dim, now.horizon.b * dim, 1.0f);
    out.horizon_share = static_cast<float>(horizon_share_);
    // m.html rowY: z 1 on the frame's foot, the screen row hud_frame_height units below its top
    out.foot_row = static_cast<float>(now.frame_top + frame_height_ * now.frame_pixels);
    out.row_power = static_cast<float>(row_power_);
    out.shown = true;
    return true;
}

}  // namespace ase::adp::godot
