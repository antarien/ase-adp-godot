#pragma once

/**
 * ASE GODOT ADAPTER - HOST GLACIER VIEW
 *
 * @file        godot_host_glacier_view.hpp
 * @brief       The ice chunks of the embedded host, as the last read of the glacier port showed them
 * @description The copy the water kinds reader keeps of every chunk with ice ase-pl-water lays on water.glacier.v1:
 *              the chunk of the grid, its edge, the ice as millimetres of water column and the melt that runs off it
 *              per hour. One purpose of its own (WRFL_ASE_TIER_WIPE, the header ratchet): only the units that draw or
 *              read the glaciers include it.
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
 * One chunk with ice as the last read of the glacier port showed it. A value the record lacks stays NaN, and a chunk
 * with any value missing draws no glacier.
 */
struct VivariumGlacierView {
    uint32_t object_id = 0;     // the chunk, its row as the plugin names it on its port
    double   chunk_x = 0.0;     // chunk grid coordinate east (glcr_chunk_x); NaN when unanswered
    double   chunk_z = 0.0;     // chunk grid coordinate north (glcr_chunk_z); NaN when unanswered
    double   chunk_m = 0.0;     // edge of the chunk, metres (glcr_chunk_m); NaN when unanswered
    double   ice_mm = 0.0;      // ice as millimetres of water column (glcr_ice_mm); NaN when unanswered
    double   melt_mm_h = 0.0;   // melt as millimetres of water column per hour (glcr_melt_mm_h); NaN when unanswered
};

}  // namespace ase::adp::godot
