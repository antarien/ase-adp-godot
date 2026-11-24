#pragma once

/**
 * ASE GODOT ADAPTER - HOST SEA VIEW
 *
 * @file        godot_host_sea_view.hpp
 * @brief       The salt chunks of the embedded host, as the last read of the sea port showed them
 * @description The copy the water kinds reader keeps of every chunk of salt water ase-pl-water lays on water.sea.v1:
 *              the chunk of the grid, its edge and the lowest ground in it. The sea surface lies at NN0, the zero of
 *              the world height of the engine; the record carries no level of its own, so none is held here. One
 *              purpose of its own (WRFL_ASE_TIER_WIPE, the header ratchet): only the units that draw or read the
 *              sea include it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * One salt chunk as the last read of the sea port showed it. A value the record lacks stays NaN, and a chunk with
 * any value missing draws no sea.
 */
struct VivariumSeaView {
    uint32_t object_id = 0;  // the chunk, its row as the plugin names it on its port
    double   chunk_x = 0.0;  // chunk grid coordinate east (sea_chunk_x); NaN when unanswered
    double   chunk_z = 0.0;  // chunk grid coordinate north (sea_chunk_z); NaN when unanswered
    double   chunk_m = 0.0;  // edge of the chunk, metres (sea_chunk_m); NaN when unanswered
    double   low_m = 0.0;    // lowest ground of the chunk, metres above NN0 (sea_low_m); NaN when unanswered
};

}  // namespace ase::adp::godot
