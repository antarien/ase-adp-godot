#pragma once

/**
 * ASE GODOT ADAPTER - HOST FIRE VIEW
 *
 * @file        godot_host_fire_view.hpp
 * @brief       One fire cell of the embedded host as the last snapshot of the fire port showed it
 * @description The copy GodotHostResourceManager keeps of every fire cell ase-fire computes and ase-pl-ingle
 *              lays out, with the air ase-atmosphere grades over it (PLAN_ASE_VIVARIUM_PHASE_06_HUD U04, B0657).
 *              One purpose of the dissolved godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header
 *              ratchet): only the units that draw the fire and its survival cards read it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.04.00024 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * One fire cell as the last snapshot of the fire port showed it - a copy (PLAN_ASE_VIVARIUM_PHASE_06_HUD U04, B0657).
 * ase-fire computes the cell (HUB_FIRE_CELL) and ase-atmosphere the air; ase-pl-ingle lays one record per cell in
 * Production (IngleHostFireExptSystem). A key the record does not carry stands as NaN. The view computes none of the
 * values.
 */
struct VivariumSurvivalFireView {
    uint32_t object_id = 0;    // the fire cell, the entity of its HUB_FIRE_CELL row
    double   flame = 0.0;      // mark HUB_FIRE_CELL_FLAM: 1 while the cell burns with a flame
    double   charcoal = 0.0;   // mark HUB_FIRE_CELL_CHAR: 1 while the cell burns as a bed of charcoal
    double   ember = 0.0;     // mark HUB_FIRE_CELL_EMBR: 1 while the cell glows as ember
    double   ash = 0.0;        // mark HUB_FIRE_CELL_ASH: 1 once the cell is ash
    double   fuel = 0.0;       // HUB_FIRE_CELL_FUEL: fuel left, fuel units of ase-fire
    double   fuel_laid = 0.0;  // HUB_FIRE_CELL_FUEL_LAID: fuel the cell was laid with
    double   heat = 0.0;       // HUB_FIRE_CELL_HEAT: heat of the cell, heat units of ase-fire
    double   body = 0.0;       // HUB_FIRE_CELL_BODY: the fuel body the cell burns, an entity id held by the double
    double   oxygen = 0.0;     // ATM_OXYGEN_FACTOR: oxygen of the air, 1 is normal
    double   toxicity = 0.0;   // ATM_TOXICITY: toxicity of the air, 0 to 1
    double   breath_state = 0.0;   // ATM_GAS_BTH_STATE: the owner's grade of the air, 0 optimal to 4 none
    double   breath_normal = 0.0;  // ATM_CONST_BREATHABILITY_NORMAL: the grade the owner gives normal air
    double   ash_kg = 0.0;     // HUB_ENT_ASH_REQ_ASH: kg of ash the cell has left since it was laid (ingle.left.v1 ash_kg)
    double   burn_left = 0.0;  // HUB_FIRE_CELL_BURN_LEFT: seconds the fuel lasts at the burn rate of this tick (ingle.left.v1 burn_left)
};

}  // namespace ase::adp::godot
