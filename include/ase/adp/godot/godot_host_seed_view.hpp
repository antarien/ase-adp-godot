#pragma once

/**
 * ASE GODOT ADAPTER - HOST SEED VIEW
 *
 * @file        godot_host_seed_view.hpp
 * @brief       One seed body of the embedded host as the last snapshot of the seed port showed it
 * @description The copy GodotHostResourceManager keeps of every seed body ase-physics moves and sets down
 *              (register N0173, usecase:100711 publish the seed places). One purpose of the dissolved
 *              godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header ratchet): only the units that
 *              draw the seeds read it.
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
 * One seed body as the last snapshot of the seed port showed it - a copy (register N0173, usecase:100711
 * publish the seed places). ase-physics moves the seed and sets it down on the ground; the place is
 * plant-local like an organ's (origin at the stem base, y up, x east, z north), and body names the patch the
 * plant stands on. The view draws the seed there and computes none of its flight.
 */
struct VivariumSeedView {
    uint32_t object_id = 0;   // the seed body, as the plugin names it on its port
    uint32_t body = 0;        // object id of the patch the plant stands on
    double   x = 0.0;         // metres east of the stem base
    double   y = 0.0;         // metres above the ground at the stem base
    double   z = 0.0;         // metres north of the stem base
};

}  // namespace ase::adp::godot
