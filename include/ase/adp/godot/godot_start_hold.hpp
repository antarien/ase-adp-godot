#pragma once

/**
 * ASE GODOT ADAPTER - THE STEP THAT HOLDS THE START SCREEN
 *
 * @file        godot_start_hold.hpp
 * @brief       Which step of the initialisation holds the start screen, in the one order of the bar
 * @description The start screen (usecase:100387 view the plant) lifts once the patches are announced, the
 *              brooks carried, the scene built, the sky and the clock known, the figure file loaded, a jump
 *              landed and the quiet frames rendered (VivariumView::follow_initialisation). The step still
 *              missing first, in the order of VivariumView::initialisation_progress, holds the screen: the log
 *              names it in held_by=, the bar shows its word from texts.hold_* (B0357). That order stands here
 *              once, so the name, the word and the hold of the screen follow from the same choice and a step
 *              added to the bar cannot stand in one of them and miss in another.
 *
 *              NO GODOT TYPE: the view fills StartHoldReading from its own state each frame
 *              (godot_vivarium_clock.cpp); the order is tested without a Godot process
 *              (tests/test_godot_start_hold.cpp).
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

#include <cstdint>

namespace ase::adp::godot {

constexpr uint8_t GODOT_HOLD_PATCHES = 0;    // the patches of the start configuration are not announced yet
constexpr uint8_t GODOT_HOLD_BROOKS = 1;     // the stream port carries fewer brooks than the start configuration
constexpr uint8_t GODOT_HOLD_SCENE = 2;      // a part of the scene does not stand yet (terrain, streams, templates, organs, focus)
constexpr uint8_t GODOT_HOLD_SKY_CLOCK = 3;  // the sky port or the clock port carried no record yet
constexpr uint8_t GODOT_HOLD_FIGURE = 4;     // the figure file still loads in the WorkerThreadPool
constexpr uint8_t GODOT_HOLD_JUMP = 5;       // a jump waits for ase-time's verdict on its request number
constexpr uint8_t GODOT_HOLD_RENDER = 6;     // every part stands; the quiet frames are rendered
constexpr uint8_t GODOT_HOLD_STEPS = 7;      // number of steps, the length of every table indexed by a step

/**
 * What the view knows of its initialisation in one frame, each true once its step no longer holds.
 */
struct StartHoldReading {
    bool patches_announced = false;  // the host announced the patches of the start configuration
    bool brooks_carried = false;     // the stream port carries at least the brooks of the start configuration
    bool scene_built = false;        // all GODOT_INIT_SCENE_PARTS parts of the scene stand
    bool sky_and_clock = false;      // the sky port and the clock port both carried their records
    bool figure_loading = false;     // the figure file still loads (this one holds while true)
    bool jumping = false;            // a jump is running (this one holds while true)
};

/**
 * The step that holds the screen: the first one still missing in the order of the bar, GODOT_HOLD_RENDER once
 * none of the others holds.
 */
[[nodiscard]] uint8_t start_hold_step(const StartHoldReading& reading);

/**
 * The name of a step as the log writes it after held_by= and prev=: "patches", "brooks", "scene", "sky_clock",
 * "figure", "jump", "render"; "render" for a value at or above GODOT_HOLD_STEPS.
 */
[[nodiscard]] const char* start_hold_name(uint8_t step);

}  // namespace ase::adp::godot
