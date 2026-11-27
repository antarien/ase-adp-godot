#pragma once

/**
 * ASE GODOT ADAPTER - HUD ACTIVITY BARS
 *
 * @file        godot_hud_activity_bars.hpp
 * @brief       What the activity bars of the game HUD show: their order, their places, their lit action and their hit
 * @description MIG_ASE_SKILL_RSM step B5, usecase:101305 show skill progress. The adapter shows the last set of
 *              avatar.skill.bar.v1 in every frame and counts nothing itself; this unit turns that set into the bars the
 *              HUD draws.
 *
 *              THE BARS are one per time slice, at most GODOT_RSM_BARS_MAX, the youngest left: the least age first,
 *              of two of the same age the later slice (the higher id). A slice that carries no number for its action,
 *              its quantity or its age is no bar: its entry was not in the snapshot the port laid.
 *              THE HEIGHT is the quantity times hud_bars_unit, never under 0 and never over hud_bars_h: a larger
 *              quantity stands at the full height, its number shows while its action is lit.
 *              THE LIT ACTION: every bar of a lit action is lit. Which actions are lit the HUD says (the bar a finger
 *              holds, the actions of the tool in the hand); this unit only marks the bars and sums their quantity.
 *              THE HIT: a touch on the strip of the bars, the strip widened by hud_bars_hit_pad all round, chooses the
 *              bar of the column under it, the first or the last bar beyond the ends of the row.
 *              THE BAND (operator order 2026-11-25): the bottom band shows the inventory slots or the bars on its whole
 *              width. A change of the bars (a new slice, a quantity grown: the slice clock runs only while the figure
 *              learns) turns it to the bars, GODOT_RSM_BAND_HOLD_SECONDS after the last change the slots return; the
 *              inventory in use (the pack open, a thing dragged) turns it to the slots at once. The switch turns it to
 *              the other content and pins it there: the timer then turns it no more, the inventory in use still turns
 *              pinned bars to the slots, and a change after a quiet hold turns pinned slots to the bars again, since a
 *              new skilling begins. Without a bar the band shows the slots.
 *
 *              NO GODOT TYPE, so the host tests drive it with any set of slices. ActivityBars
 *              (godot_hud_activity_bars_draw.hpp) draws it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_074
 * @category    ecs/module
 * @usecase     usecase:101305 - show skill progress
 * @actor       Skill Measurement Tracker (SKILMEATRA)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.02.00002 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/** One time slice as the bars read it: the values of one record of avatar.skill.bar.v1; a missing key is NaN. */
struct ActivityBar {
    uint32_t id = 0;            // the slice entity: the later the slice, the higher the id
    double action = 0.0;        // BDI_CONST_ACTION_*: the action of the slice, the skill_id of its entry
    double qnt = 0.0;           // the quantity of the slice
    double age = 0.0;           // seconds since the slice began; NaN without its entry
};

/** Where the bars stand, in mockup units: the tokens hud_bars_* of SHA. */
struct ActivityBarsLayout {
    double x = 0.0;             // hud_bars_x: the left edge of the first bar
    double base = 0.0;          // hud_bars_base: the foot of every bar
    double w = 0.0;             // hud_bars_w: the width of a bar
    double gap = 0.0;           // hud_bars_gap: the space between two bars
    double unit = 0.0;          // hud_bars_unit: the height of one quantity
    double h = 0.0;             // hud_bars_h: the tallest bar
    double pad = 0.0;           // hud_bars_hit_pad: how far round the strip a touch still reaches it
};

/** One bar as the HUD draws it. */
struct ActivityBarPlace {
    uint32_t id = 0;            // the slice entity
    uint32_t action = 0;        // BDI_CONST_ACTION_* of the slice
    double qnt = 0.0;           // the quantity of the slice
    double x = 0.0;             // the left edge
    double y = 0.0;             // the top: base - height
    double w = 0.0;             // the width
    double h = 0.0;             // the height, 0 to hud_bars_h
    bool lit = false;           // its action is lit
};

/** At most `limit` bars of `in` that carry a number for action, quantity and age, the youngest first, in `out`. */
void activity_bars_order(const std::vector<ActivityBar>& in, uint32_t limit, std::vector<ActivityBar>& out);

/** The places of `ordered` left to right under `layout`, each lit where its action is one of `lit`, in `out`. */
void activity_bars_place(const std::vector<ActivityBar>& ordered, const ActivityBarsLayout& layout,
                         const std::vector<uint32_t>& lit, std::vector<ActivityBarPlace>& out);

/** The strip a touch reaches for `count` bars (x, y the top left, w, h); false where there is no bar. */
bool activity_bars_strip(const ActivityBarsLayout& layout, size_t count, double& x, double& y, double& w, double& h);

/** The index in `places` of the bar a touch at (`px`, `py`) chooses; false off the strip or with no bar. */
bool activity_bars_hit(const std::vector<ActivityBarPlace>& places, const ActivityBarsLayout& layout, double px,
                       double py, size_t& out);

/** The sum of the quantities of the lit bars of `places`; 0 where none is lit. */
double activity_bars_lit_qnt(const std::vector<ActivityBarPlace>& places);

/** What the bottom band shows and since when: the state the HUD keeps from frame to frame. */
struct ActivityBandState {
    uint32_t shows = 0u;        // GODOT_RSM_BAND_*: the content of the band; 0 is GODOT_RSM_BAND_SLOTS
    bool pinned = false;        // the switch chose the content: the timer turns it no more
    bool seen = false;          // a set of bars was read once; the first set is no change, the band starts on the slots
    uint64_t sign = 0u;         // activity_bars_sign of the bars of the last frame
    double changed = 0.0;       // the wall seconds of the last change of the bars
};

/** What one frame tells the band. */
struct ActivityBandInput {
    double wall = 0.0;          // the wall seconds of this frame
    uint64_t sign = 0u;         // activity_bars_sign of this frame's bars
    size_t count = 0u;          // the bars of this frame, after activity_bars_order
    bool needs_slots = false;   // the pack stands open or a thing is dragged: the inventory is in use
    bool toggled = false;       // the switch was tapped since the last frame
};

/** The sign of the bars `ordered`: their ids and quantities, never their age, which grows every frame. */
uint64_t activity_bars_sign(const std::vector<ActivityBar>& ordered);

/** The band after the frame `in`, held `hold_seconds` by the bars after their last change, in `state`. */
void activity_band_step(const ActivityBandInput& in, double hold_seconds, ActivityBandState& state);

}  // namespace ase::adp::godot
