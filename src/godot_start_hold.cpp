/**
 * ASE GODOT ADAPTER - THE STEP THAT HOLDS THE START SCREEN
 *
 * @file        godot_start_hold.cpp
 * @brief       Which step of the initialisation holds the start screen, in the one order of the bar
 * @description See godot_start_hold.hpp. The steps are tested in the order of the bar; the first one that
 *              still holds is the answer.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.26.00026 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_start_hold.hpp>

namespace ase::adp::godot {

uint8_t start_hold_step(const StartHoldReading& reading) {
    if (!reading.patches_announced) {
        return GODOT_HOLD_PATCHES;
    }
    if (!reading.brooks_carried) {
        return GODOT_HOLD_BROOKS;
    }
    if (!reading.scene_built) {
        return GODOT_HOLD_SCENE;
    }
    if (!reading.sky_and_clock) {
        return GODOT_HOLD_SKY_CLOCK;
    }
    if (reading.figure_loading) {
        return GODOT_HOLD_FIGURE;
    }
    if (reading.jumping) {
        return GODOT_HOLD_JUMP;
    }
    return GODOT_HOLD_RENDER;
}

const char* start_hold_name(uint8_t step) {
    // one name per step, in the order of the GODOT_HOLD_* values
    static const char* const names[GODOT_HOLD_STEPS] = {"patches", "brooks", "scene", "sky_clock",
                                                        "figure",  "jump",   "render"};
    return names[step < GODOT_HOLD_STEPS ? step : GODOT_HOLD_RENDER];
}

}  // namespace ase::adp::godot
