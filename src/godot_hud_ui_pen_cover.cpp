/**
 * ASE GODOT ADAPTER - HUD UI PEN COVER
 *
 * @file        godot_hud_ui_pen_cover.cpp
 * @brief       The coverage rule of the HUD's strokes: the core Godot draws and the feather it attaches to it
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_pen_cover.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    B0678
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_cover.hpp>

namespace ase::adp::godot {

float pen_core(float width) {
    if (width < 0.0f) {
        return width;
    }
    // A core of at least 1 carries the whole feather (core + F covers `width`); below it Godot scales the feather
    // by the core, so core * (1 + F) covers it. Both meet at a core of 1.
    if (width >= 1.0f + GODOT_PEN_FEATHER_PX) {
        return width - GODOT_PEN_FEATHER_PX;
    }
    return width / (1.0f + GODOT_PEN_FEATHER_PX);
}

float pen_feather(float core) {
    if (core < 0.0f) {
        return 0.0f;
    }
    return core >= 1.0f ? GODOT_PEN_FEATHER_PX : GODOT_PEN_FEATHER_PX * core;
}

}  // namespace ase::adp::godot
