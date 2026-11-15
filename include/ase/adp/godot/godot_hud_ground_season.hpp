#pragma once

/**
 * ASE GODOT ADAPTER - HUD GROUND SEASON
 *
 * @file        godot_hud_ground_season.hpp
 * @brief       The colour of the ground by season, snow, depth and horizon, derived for the 3D scene
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, row N06 (m.html groundCol 426 to 428, design_tokens.json
 *              section VIVARIUM GROUND SEASON). The mockup paints every row of the ground in the colour of the
 *              season, GROUND_SEASON_GLACIX, _SALTATRIX, _IGNIX or _HALITRIX in the order of CAL_CONST_SEASON_*;
 *              while it snows (the weather GODOT_HUD_WEATHER_SNOW) that colour turns toward GROUND_SNOW by
 *              ground_snow_share. A row at depth z is darkened to ground_depth_floor + ground_depth_span * z and
 *              blended toward the horizon's colour times ground_horizon_dim by ground_horizon_share * (1 - z). z is a
 *              row of the screen as the mockup's rowY lays it (m.html 358, HOR + z ^ ground_row_power * 252): 0 on
 *              the row where the square's far edge stands, 1 on the foot of the HUD frame, hud_frame_height units
 *              below its top. The block derives the colours, the shares and the frame's foot row as a shade; the
 *              scene paints the ground from it (light_plant_ground_season, godot_plant_ground_season.hpp), where the
 *              sun and the ambient light take the place of the mockup's light(). Nothing of the ground is changed
 *              here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0460 (flora.day.v1: the season), N0449 (flora.weather.v1: the snow), carrier N0448
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_plant_ground_season.hpp>  // GroundSeasonShade, the shade the scene paints the ground from

#include <godot_cpp/variant/color.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the ground's colour follows this frame. */
struct GroundSeasonFrame {
    int32_t season = -1;           // REQUIRED: the season, CAL_CONST_SEASON_GLACIX .. HALITRIX: the clock's, else the bridge's (N0460)
    const char* weather = nullptr; // the weather key (GODOT_HUD_WEATHER_*) of the port or of the bridge (N0449); nullptr: no snow
    ::godot::Color horizon;        // the horizon's colour as the sky shader draws it this frame
    double frame_top = 0.0;        // the screen row of the HUD frame's top, pixels
    double frame_pixels = 0.0;     // REQUIRED: pixels of the screen per mockup unit (the HUD frame's scale)
};

/** The colour of the ground by season, snow, depth and horizon. */
class GroundSeason {
public:
    /** Binds the seven measures and the five colours; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The shade of `now`; false with the reason in `failed` for a season without colour or a frame without scale. */
    bool shade(const GroundSeasonFrame& now, GroundSeasonShade& out, std::string& failed) const;

private:
    double snow_share_ = 0.0;      // ground_snow_share
    double depth_floor_ = 0.0;     // ground_depth_floor
    double depth_span_ = 0.0;      // ground_depth_span
    double horizon_share_ = 0.0;   // ground_horizon_share
    double horizon_dim_ = 0.0;     // ground_horizon_dim
    double row_power_ = 0.0;       // ground_row_power: the power of the mockup's rowY
    double frame_height_ = 0.0;    // hud_frame_height: the frame's foot, mockup units below its top
    std::vector<::godot::Color> seasons_;  // GROUND_SEASON_* in the order of CAL_CONST_SEASON_*
    ::godot::Color snow_;                  // GROUND_SNOW
};

}  // namespace ase::adp::godot
