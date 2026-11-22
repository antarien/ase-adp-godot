/**
 * ASE GODOT ADAPTER - HUD LEAF SEASON
 *
 * @file        godot_hud_leaf_season.cpp
 * @brief       The colour of the leaf by season and drought, derived for the 3D scene
 * @description See godot_hud_leaf_season.hpp.
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

#include <ase/adp/godot/godot_hud_leaf_season.hpp>

namespace ase::adp::godot {

bool LeafSeason::bind(const UiTheme& theme, std::string& failed) {
    if (!theme.measure("leaf_dry_share", dry_share_)) {
        failed = "leaf_dry_share";
        return false;
    }
    // the olive turns a wilting leaf from none to all of it
    if (!(dry_share_ >= 0.0 && dry_share_ <= 1.0)) {
        failed = "leaf_dry_share must lie in 0..1";
        return false;
    }
    seasons_.clear();
    for (const char* name : {"LEAF_SEASON_GLACIX", "LEAF_SEASON_SALTATRIX", "LEAF_SEASON_IGNIX",
                             "LEAF_SEASON_HALITRIX"}) {
        ::godot::Color colour;
        if (!theme.color(name, colour)) {
            failed = name;
            return false;
        }
        seasons_.push_back(colour);
    }
    if (!theme.color("LEAF_DRY", dry_)) {
        failed = "LEAF_DRY";
        return false;
    }
    return true;
}

bool LeafSeason::shade(const LeafSeasonFrame& now, LeafSeasonShade& out, std::string& failed) const {
    out = LeafSeasonShade{};
    if (now.season < 0 || static_cast<size_t>(now.season) >= seasons_.size()) {
        failed = "LeafSeason: the season " + std::to_string(now.season) + " has no colour in LEAF_SEASON_*";
        return false;
    }
    // the season's colour as the token gives it: its alpha is the share it takes of the leaf, 0 in GLACIX, where the
    // birch stands bare and a leaf that still hangs keeps its own colour
    out.season = seasons_[static_cast<size_t>(now.season)];
    // the olive with leaf_dry_share as its alpha; the shader lays it over a leaf only while its stand wilts
    out.dry = ::godot::Color(dry_.r, dry_.g, dry_.b, static_cast<float>(dry_share_));
    // in autumn the season's colour is the autumn colour, and every birch reaches it at its own pace: the shader turns
    // a leaf whose body carries its autumn colour by that share instead of the season's (usecase:101646 colour the
    // leaves in autumn)
    out.autumn = now.autumn ? 1.0f : 0.0f;
    out.shown = true;
    return true;
}

}  // namespace ase::adp::godot
