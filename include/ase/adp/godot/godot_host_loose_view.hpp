#pragma once

/**
 * ASE GODOT ADAPTER - HOST LOOSE VIEW
 *
 * @file        godot_host_loose_view.hpp
 * @brief       The looseness raster of the soil as the last snapshot of the loose port showed it, cell by cell
 * @description The copy GodotHostResourceManager keeps of terrain.loose.v1 (PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.9,
 *              register N0456 under carrier N0448): ase-terrain, the owner of the soil's compaction, writes the
 *              looseness of every cell of its ase-gis raster layer (CAUSA_ASE_GIS LYR_LITHO_COMPACTION, looseness 0
 *              to 1 after the spade, settled by rain at the owner) and files one record per cell with the cell's
 *              centre and edge; flora reads the soil, it never keeps it. The view keeps the cells as the grid they form, row by row from the south-west
 *              cell; a reader asks its cell (GodotHostResourceManager::loose_at) and the crumbs draw the grid. While
 *              the port is not offered or files no cell, `known` stays false and the HUD's loose bridge draws the
 *              mockup's grid. One purpose of the dissolved godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the
 *              header ratchet): only the units that read the loosened soil read it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @register    N0456 (flora.loose.v1), carrier N0448
 * @created     2026-11-22
 * @modified    2026-11-22
 * @version     00.00.04.00024 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/** The looseness raster of flora.loose.v1; `known` stays false while the port is not offered or files no cell. */
struct VivariumLooseView {
    bool known = false;         // a complete raster stood in the last snapshot: every cell of its grid exactly once
    std::vector<double> loose;  // the looseness 0..1 per cell, row * columns + column, from the south-west cell
    int64_t columns = 0;        // cells east
    int64_t rows = 0;           // cells north
    double west_x = 0.0;        // centre of the south-west cell, world metres east (key x)
    double south_z = 0.0;       // centre of the south-west cell, world metres north (key z)
    double cell_m = 0.0;        // edge of one cell, metres, the same for every cell (key cell_m)
};

}  // namespace ase::adp::godot
