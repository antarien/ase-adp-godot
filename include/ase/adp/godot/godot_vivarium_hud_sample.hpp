#pragma once

/**
 * ASE GODOT ADAPTER - THE LAST SAMPLE THE HUD TOOK OF THE SIMULATION
 *
 * @file        godot_vivarium_hud_sample.hpp
 * @brief       The records the HUD read at its last sample: the clock, the patch in focus, the night and the host's time
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.1: the HUD samples the simulation at hud_refresh_ms (UiRefresh,
 *              T10, m.html 1037), and again at once when the place in focus or the stage, condition, wilt or death of
 *              its patch changed (godot_vivarium_hud.cpp). Between two samples every frame draws what the last one
 *              read: the band and the sheet the clock record and the night, the cards the patch record, the debug
 *              layer the simulation seconds and the ticks (godot_vivarium_hud_draw.cpp), the bridge's glass the pace
 *              of the clock (godot_vivarium_hud_bridge_clock.cpp).
 *
 *              It stands apart from the view because it is one responsibility with its own life: written by the
 *              sample, read by the frames up to the next one. The view holds one of it.
 *
 *              NO GODOT TYPE: plain values and the host's record views only.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-22
 * @modified    2026-11-22
 * @version     00.00.23.00023 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_clock_view.hpp>
#include <ase/adp/godot/godot_host_patch_view.hpp>
#include <ase/adp/godot/host/types.hpp>

#include <cstdint>

namespace ase::adp::godot {

/** What the HUD read at its last sample; place GODOT_NO_PLACE: no sample found a place in focus yet. */
struct VivariumHudSample {
    VivariumClockView clock;                 // the clock record: the band, the sheet and the debug read it
    VivariumPatchView patch;                 // the patch record of the place in focus
    bool              patch_known = false;   // the sample found a record for the place in focus
    uint32_t          place = GODOT_NO_PLACE;  // the place in focus
    bool              night = false;         // the sun under ase-pl-sky's horizon
    double            simulation_s = 0.0;    // the host's simulation seconds (debug {sim})
    uint64_t          steps = 0;             // the host's ticks (debug {steps})
};

}  // namespace ase::adp::godot
