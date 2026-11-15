#pragma once

/**
 * ASE GODOT ADAPTER - HUD CONTEXT
 *
 * @file        godot_hud_context.hpp
 * @brief       Which chips of the HUD the view shows and where: the rules of the context, no drawing
 * @description The HUD offers a chip only where it means something. A chip that acts on a patch shows
 *              once a place is selected; a chip of the debug layer shows only while the debug toggle is
 *              on. Both rules stand per chip in the `hud.chips` list of vivarium_start.json, and this
 *              unit only applies them to the state the view is in.
 *
 *              THE SLOT IS THE ORDER OF THE LIST. Visible chips take the slots 0, 1, 2 ... in the order
 *              hud.chips names them, so the spine never shows a gap where a hidden chip would stand. A
 *              hidden chip gets no slot (UINT32_MAX) and is never running, even when the state names it
 *              as the running one - a tool whose chip is gone has nothing the player can stop.
 *
 *              No Godot type: the unit stands in the host library ase-adp-godot-host and its test runs
 *              without an engine (tests/test_godot_hud_context.cpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-09
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/** The two rules of one chip as hud.chips states them. */
struct HudChipRule {
    bool needs_place = false;  // the chip acts on a patch: shown only once a place is selected
    bool debug_only = false;   // the chip belongs to the debug layer: shown only while debug is on
};

/** The state of the view the rules are applied to. */
struct HudViewState {
    bool place_selected = false;         // a patch is selected
    bool debug_on = false;               // the debug toggle is on
    uint32_t running_chip = UINT32_MAX;  // index into hud.chips of the tool that runs, UINT32_MAX = none
};

/** What the view does with one chip. */
struct HudChipShow {
    bool visible = false;        // the chip is drawn and takes taps
    bool running = false;        // the chip's tool runs: the chip draws its running ring
    uint32_t slot = UINT32_MAX;  // position on the spine, from 0, UINT32_MAX = hidden
};

/**
 * Applies `rules` to `state`: `shown` gets one entry per rule, in the same order. Visible chips take
 * contiguous slots from 0 in list order; a hidden chip keeps slot UINT32_MAX and running false.
 */
void hud_context_resolve(const std::vector<HudChipRule>& rules, const HudViewState& state,
                         std::vector<HudChipShow>& shown);

}  // namespace ase::adp::godot
