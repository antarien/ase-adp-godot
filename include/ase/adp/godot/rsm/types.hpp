#pragma once
/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the tab layers of the debug console and the numbers of the RSM table
 * @description All compile-time constants of the rsm sub-type; NO runtime values - those belong in the views.
 *              The three tab layers of the debug console at the foot (SYSTEM with the tabs, the RSM table with its
 *              four rings, STATUS with the four status lines), the two contents of the bottom band (the inventory
 *              slots or the activity bars) and how long the bars hold it, the range of the action codes (BDI_CONST_ACTION_*, an entry's skill_id) and of the phases
 *              of a learning entry of avatar.skill.v1,
 *              and the numbers of the clock format of the prototype (CountdownTimerFormater.format: HH:MM:SS).
 *              Read by the debug layer (godot_hud_debug_layer.cpp), the table (godot_hud_debug_rsm.cpp and
 *              godot_hud_debug_rsm_draw.cpp), the HUD that holds the choice of the view (godot_hud_compose.cpp) and the
 *              activity bars of the game HUD (godot_hud_activity_bars.cpp and godot_hud_activity_bars_draw.cpp, B5) and
 *              its two skill rings DCD and ACD (godot_hud_skill_rings_draw.cpp, B5).
 *
 *              WHY A TYPES HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); these constants have four
 *              readers, so a change to one compiles those units again, never every unit that draws.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_074
 * @usecase     usecase:101305 - show skill progress
 * @actor       Skill Measurement Tracker (SKILMEATRA)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.02.00002 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: RSM = the table of the timers of a learning entry; SES = session seconds; ACD = fluctuation_current;
 * TUC = time under condition, seconds; DCD = remaining grace seconds; QNT = seconds left in the running time slice;
 * TUL = time until the lock lifts; QUE = the fill of the queue; LCK = lock seconds; EXP = dgEXP.
 */

/**
 * The tab layers of the debug console at the foot, in the order of texts.hud.debug_views: the open drawer shows
 * exactly one of them, nothing of the console stands over the scene.
 */
constexpr uint32_t GODOT_HUD_DEBUG_VIEW_SYSTEM = 0u;   // the controls: the tabs TEMPO, SALTO, DATA, RELEVO
constexpr uint32_t GODOT_HUD_DEBUG_VIEW_RSM = 1u;      // the timer table of the learning entries, always shown, no ring
constexpr uint32_t GODOT_HUD_DEBUG_VIEW_STATUS = 2u;   // the four status lines: build, clock, patch, salt
constexpr uint32_t GODOT_HUD_DEBUG_VIEWS = 3u;         // tab layers of the console

/**
 * The learning entries of avatar.skill.v1 the table shows.
 */
constexpr uint32_t GODOT_RSM_PHASES = 5u;              // phases of an entry, 0 to 4: the words texts.hud.debug_rsm.phase.<n>
constexpr uint32_t GODOT_RSM_ACTION_FIRST = 150u;      // the first action code an entry carries as skill_id: BDI_CONST_ACTION_MOVE_TO; the words texts.hud.debug_rsm.act.<code>
constexpr uint32_t GODOT_RSM_ACTION_LAST = 171u;       // the last action code: BDI_CONST_ACTION_HEAP; a code of the span no BDI_CONST_ACTION_* carries has no word
constexpr uint32_t GODOT_RSM_VALUE_ROWS = 13u;         // rows of a column of numbers: SES ACD TUC DCD QNT TUL, QUE LCK EXP PRG VEL MSR VOL
constexpr int32_t GODOT_RSM_RATE_DECIMALS = 2;         // places after the point of the VEL row: the smoothed rate of experience is a fraction per second
constexpr uint32_t GODOT_RSM_ROWS_ABOVE_RULE = 6u;     // the rows above the divider: SES ACD TUC DCD QNT TUL
constexpr uint32_t GODOT_RSM_GRID_ROWS = 6u;           // cells of a column of the prototype grid: SES ACD TUC DCD QNT TUL, the first six value rows
constexpr uint32_t GODOT_RSM_RED_PHASE = 0u;           // the phase word index whose SES cell is drawn red: texts.hud.debug_rsm.phase.0, the value HUB_SKL_RSM_PHASE carries while the entry learns

/**
 * The clock of the prototype: CountdownTimerFormater.format, seconds as HH:MM:SS.
 */
constexpr uint32_t GODOT_RSM_SECONDS_PER_MINUTE = 60u; // seconds of a minute
constexpr uint32_t GODOT_RSM_MINUTES_PER_HOUR = 60u;   // minutes of an hour
constexpr double GODOT_RSM_PERCENT = 100.0;            // PRG of the server (0 to 100) as the share of 0 to 1 the per cent row takes

/**
 * The activity bars of the game HUD (MIG_ASE_SKILL_RSM B5): one bar per time slice of avatar.skill.bar.v1.
 */
constexpr uint32_t GODOT_RSM_BARS_MAX = 20u;             // bars at most, as the prototype's twenty: the twenty youngest slices
constexpr uint32_t GODOT_RSM_ACTION_NONE = UINT32_MAX;   // no action: nothing is lit, a bar of no known action
constexpr const char* GODOT_RSM_ACT_BARS = "skill_bars"; // the act a touch on the bars names: it lights their action while held and asks the world nothing

/**
 * The bottom band: it shows the inventory (bag, holsters, hand) or the activity bars on its whole width, by what the
 * player does (operator order 2026-11-25): skilling shows the bars, the inventory in use shows the slots.
 */
constexpr uint32_t GODOT_RSM_BAND_SLOTS = 0u;             // the band shows the inventory row and the small strip of bars right of the hand
constexpr uint32_t GODOT_RSM_BAND_BARS = 1u;              // the band shows the bars on its whole width, the inventory row stands hidden
constexpr double GODOT_RSM_BAND_HOLD_SECONDS = 10.0;      // wall seconds the bars hold the band after their last change, then the slots return
constexpr const char* GODOT_RSM_ACT_BAND = "skill_band";  // the act of the band switch: it turns the band to the other content and holds it there

/**
 * The skill rings of the game HUD (MIG_ASE_SKILL_RSM B5, operator order 2026-11-25): the two wheels of the prototype,
 * the remaining time of the learning entries, in the free corner right of the sky arc.
 */
constexpr uint32_t GODOT_RSM_RING_DCD = 0u;               // the upper ring: the largest share of the grace left, texts.hud.skill_ring.dcd
constexpr uint32_t GODOT_RSM_RING_ACD = 1u;               // the lower ring: the largest share of the fluctuation, texts.hud.skill_ring.acd
constexpr uint32_t GODOT_RSM_RINGS = 2u;                  // rings of the game HUD, as the prototype's two wheels

}  // namespace ase::adp::godot
