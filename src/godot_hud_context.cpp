/**
 * ASE GODOT ADAPTER - HUD CONTEXT
 *
 * @file        godot_hud_context.cpp
 * @brief       Which chips of the HUD the view shows and where: the rules of the context, no drawing
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.2, see godot_hud_context.hpp.
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

#include <ase/adp/godot/godot_hud_context.hpp>

namespace ase::adp::godot {

void hud_context_resolve(const std::vector<HudChipRule>& rules, const HudViewState& state,
                         std::vector<HudChipShow>& shown) {
    shown.assign(rules.size(), HudChipShow{});
    uint32_t next_slot = 0;
    for (uint32_t index = 0; index < rules.size(); ++index) {
        const HudChipRule& rule = rules[index];
        const bool place_met = !rule.needs_place || state.place_selected;
        const bool debug_met = !rule.debug_only || state.debug_on;
        if (!place_met || !debug_met) {
            continue;
        }
        HudChipShow& show = shown[index];
        show.visible = true;
        show.running = state.running_chip == index;
        show.slot = next_slot;
        ++next_slot;
    }
}

}  // namespace ase::adp::godot
