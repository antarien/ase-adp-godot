#pragma once

/**
 * ASE GODOT ADAPTER - HUD LEAF SEASON
 *
 * @file        godot_hud_leaf_season.hpp
 * @brief       The colour of the leaf by season and drought, derived for the 3D scene
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, register B0278 (m.html leafCol 486, design_tokens.json
 *              section VIVARIUM LEAF SEASON). The mockup paints the leaf in the colour of the season,
 *              LEAF_SEASON_GLACIX, _SALTATRIX, _IGNIX or _HALITRIX in the order of CAL_CONST_SEASON_*, and turns it
 *              toward LEAF_DRY by leaf_dry_share while the soil lies under hud_bridge_sign_wilt_soil_rel, the
 *              threshold at which the stand wilts. The block derives the season's colour and the olive with its
 *              share as a shade; the scene paints the leaf from it (light_plant_leaf_season,
 *              godot_plant_leaf_season.hpp), and the shader takes the olive only for a wilting stand. Nothing of the
 *              plant is changed here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    B0278, N0460 (flora.day.v1: the season)
 * @created     2026-11-13
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_plant_leaf_season.hpp>  // LeafSeasonShade, the shade the scene paints the leaf from

#include <godot_cpp/variant/color.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the leaf's colour follows this frame. */
struct LeafSeasonFrame {
    int32_t season = -1;  // REQUIRED: the season, CAL_CONST_SEASON_GLACIX .. HALITRIX: the clock's, else the bridge's (N0460)
};

/** The colour of the leaf by season and drought. */
class LeafSeason {
public:
    /** Binds the measure and the five colours; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The shade of `now`; false with the reason in `failed` for a season without colour. */
    bool shade(const LeafSeasonFrame& now, LeafSeasonShade& out, std::string& failed) const;

private:
    double dry_share_ = 0.0;               // leaf_dry_share
    std::vector<::godot::Color> seasons_;  // LEAF_SEASON_* in the order of CAL_CONST_SEASON_*
    ::godot::Color dry_;                   // LEAF_DRY
};

}  // namespace ase::adp::godot
