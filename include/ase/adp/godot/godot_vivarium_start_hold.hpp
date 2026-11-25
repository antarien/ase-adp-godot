#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM START HOLD RECORD
 *
 * @file        godot_vivarium_start_hold.hpp
 * @brief       VivariumStartHold - what the view counts while the start screen stands: quiet frames, pipelines, the hold
 * @description The start screen (usecase:100387 view the plant) lifts once the world stands complete and the renderer
 *              drew GODOT_INIT_RENDER_FRAMES frames in a row without a new pipeline (godot_vivarium_clock.cpp,
 *              follow_initialisation). Which step holds it is the lesson of godot_start_hold.hpp, without a Godot
 *              type; this record is the view's memory between two frames of that watch: the quiet frames counted,
 *              the pipeline compilations of the previous frame, the step that held the screen, its word on the bar
 *              and since when it holds. boot_host and a jump clear it, so every boot names its hold anew (B0357).
 *
 *              IT STEERS THE VIEW, unlike the records of godot_vivarium_watch.hpp: render_frames lifts the screen
 *              and hold_word is drawn on the bar (godot_vivarium_start.cpp). It stands apart from
 *              godot_vivarium_node.hpp because only the start screen's units change it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <godot_cpp/variant/string.hpp>

#include <cstdint>

namespace ase::adp::godot {

/** The start screen's watch between two frames (follow_initialisation). */
struct VivariumStartHold {
    uint32_t        render_frames = 0;  // frames rendered in a row under the start screen with the world complete and no new pipeline
    uint64_t        pipelines = 0;      // pipeline compilations of the rendering server counted in the previous frame
    ::godot::String hold;               // the step that held the start screen in the previous frame, empty before the first (B0357)
    ::godot::String hold_word;          // its word from texts.hold_*, drawn after the percent line of the bar
    uint64_t        hold_since_ms = 0;  // wall-clock milliseconds (Time::get_ticks_msec) at which that step began to hold
};

}  // namespace ase::adp::godot
