#pragma once

/**
 * ASE GODOT ADAPTER - HOST GROUNDWATER VIEW
 *
 * @file        godot_host_groundwater_view.hpp
 * @brief       The groundwater of every chunk of the embedded host, as the last read of the groundwater port showed it
 * @description The copy GodotHostResourceManager keeps of every chunk ase-pl-hydrogeology lays on
 *              hydrogeology.groundwater.v1 (HydrogeologyHostGndwExptSystem): the chunk, the water table against its
 *              mean ground, the standing water, the stock and the flow it gives the brooks. One purpose of its own
 *              (WRFL_ASE_TIER_WIPE, the header ratchet): only the units that draw or read the groundwater include it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-22
 * @modified    2026-11-22
 * @version     00.00.04.00024 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * The groundwater of one chunk as the last read of the groundwater port showed it (usecase:100400 feed the
 * groundwater, computed by ase-hydro HydroChnkGndwCalcSystem and answered by the ase-gis cell under HUB_TERN_CHK).
 * The chunk grid coordinates come with every record; a value the cell never answered is missing from the record,
 * and its field stays NaN. The view computes none of these values.
 */
struct VivariumGroundwaterView {
    uint32_t object_id = 0;     // the chunk, its HUB_TERN_CHK row as the plugin names it on its port
    double   chunk_x = 0.0;     // chunk grid X of the row
    double   chunk_y = 0.0;     // chunk grid Y of the row, the vertical slice
    double   chunk_z = 0.0;     // chunk grid Z of the row
    double   table_m = 0.0;     // depth of the water table below the mean ground of the chunk, metres, negative above it; NaN when unanswered
    double   depth_m = 0.0;     // standing water over the lowest ground of the chunk, metres; NaN when unanswered
    double   stock_m3 = 0.0;    // groundwater stock of the chunk against its rest table, cubic metres; NaN when unanswered
    double   bank_m3_s = 0.0;   // water the stock gives the brooks crossing the chunk, m3/s, negative where it draws; NaN when unanswered
};

}  // namespace ase::adp::godot
