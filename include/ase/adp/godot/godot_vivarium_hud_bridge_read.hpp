/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD BRIDGE READ
 *
 * @file        godot_vivarium_hud_bridge_read.hpp
 * @brief       read_bridge_in - one hud_bridge_* token of SHA within its range, the reader every bridge unit shares
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5: the bridge units (godot_vivarium_hud_bridge.cpp for the flora
 *              groups, godot_vivarium_hud_bridge_survival.cpp for the survival groups) load the mockup's values from
 *              the hud_bridge_* tokens of SHA (theme.css --ase-hud-bridge-*, staged as res://config/design_tokens.json
 *              and read through UiTheme). A token outside its range refuses the configuration with its name.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-20
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#pragma once

#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <string>

namespace ase::adp::godot {

/** The token `token` into `out`, which must lie in [low, high]; false with the token's name in `failed`. */
inline bool read_bridge_in(const UiTheme& theme, const char* token, double low, double high, double& out,
                           std::string& failed) {
    if (!theme.measure(token, out) || out < low || out > high) {
        failed = token;
        return false;
    }
    return true;
}

}  // namespace ase::adp::godot
