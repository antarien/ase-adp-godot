#pragma once

/**
 * ASE GODOT ADAPTER - HOST RACK VIEW
 *
 * @file        godot_host_rack_view.hpp
 * @brief       One tool rack body of the embedded host as the last snapshot of the rack port showed it
 * @description The copy GodotHostResourceManager keeps of every tool rack ase-physics holds as a prop body and
 *              ase-pl-toolrack lays patch-local (usecase:101625 carry a tool in hand). One purpose of the
 *              dissolved godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header ratchet): only the
 *              units that draw the rack and its tools read it.
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
 * One tool rack body as the last snapshot of the rack port showed it - a copy (usecase:101625 carry a tool in
 * hand). ase-physics holds the rack as a prop body and sets it down on the ground; ase-pl-toolrack lays its
 * place patch-local (origin at the stem foot, y up, x east, z north), and object_id names the patch whose frame
 * the rack stands in. The view draws the rack and its tools there and computes none of its height.
 */
struct VivariumRackView {
    uint32_t object_id = 0;   // the patch the rack stands at, as the plugin names it on its port
    double   x = 0.0;         // metres east of the stem foot
    double   y = 0.0;         // metres above the stem foot, the rest height ase-physics found
    double   z = 0.0;         // metres north of the stem foot
};

}  // namespace ase::adp::godot
