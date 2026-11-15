#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI RNG
 *
 * @file        godot_hud_ui_rng.hpp
 * @brief       The mockup's rng(a): the draws in the mockup's order wherever the HUD lays what the mockup scatters
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD building block Basis. The mockup scatters by its rng(a), mulberry32
 *              over a 32-bit state (VIVARIUM_PHASE_06_HUD_V2.html 364): the roots of drawRootCut from one seed
 *              (VivariumHandBridge), the crumbs of broken-up soil from row * 97 + column (GroundCrumbs). Every
 *              unit that lays such a scatter draws from here, so the same seed lays the same points the mockup
 *              lays; Math.imul's wrap is the wrap of uint32_t.
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>

namespace ase::adp::godot {

/** The mockup's rng(a), mulberry32: the next draw in [0, 1) from `state`, which it advances. */
inline double ui_rng_draw(uint32_t& state) {
    state += 0x6D2B79F5u;
    uint32_t t = (state ^ (state >> 15u)) * (1u | state);
    t = (t + ((t ^ (t >> 7u)) * (61u | t))) ^ t;
    return static_cast<double>(t ^ (t >> 14u)) / 4294967296.0;
}

}  // namespace ase::adp::godot
