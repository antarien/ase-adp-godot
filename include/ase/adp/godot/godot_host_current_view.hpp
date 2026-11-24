#pragma once

/**
 * ASE GODOT ADAPTER - HOST CURRENT VIEW
 *
 * @file        godot_host_current_view.hpp
 * @brief       The flowing water of the river cells of the embedded host, as the last read of the current port showed it
 * @description The copy GodotHostResourceManager keeps of every current cell ase-pl-water lays on water.current.v1
 *              (WaterHostCurrExptSystem): the centre of the cell, the depth and speed of its flowing water and the
 *              direction it runs in. One purpose of its own (WRFL_ASE_TIER_WIPE, the header ratchet): only the units
 *              that draw or read the river current include it.
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
 * One current cell as the last read of the current port showed it (usecase:100399 carry brook water, computed by
 * ase-hydro HydroCurrRotCalcSystem and answered by the ase-gis cell). The plugin lays the place and every value the
 * cell answered; a value the cell never answered is missing from the record, and its field stays NaN. The view
 * computes none of these values.
 */
struct VivariumCurrentView {
    uint32_t object_id = 0;    // the current cell, its HUB_HYDR_CURR row as the plugin names it on its port
    double   x = 0.0;          // centre of the cell, world metres east (curr_x)
    double   z = 0.0;          // centre of the cell, world metres north (curr_z)
    double   depth_m = 0.0;    // depth of the flowing water over the ground of the cell, metres; NaN when unanswered
    double   speed_m_s = 0.0;  // speed of the water in the cell, metres per second; NaN when unanswered
    double   east = 0.0;       // share of the flow direction towards east, -1 to 1; NaN when unanswered
    double   north = 0.0;      // share of the flow direction towards north, -1 to 1; NaN when unanswered
    double   bloom_kg_m3 = 0.0;  // algae dry mass in the water of the cell, kg/m3 (water.quality.v1, D2508); NaN when unanswered
    double   rot_kg_m3 = 0.0;    // dissolved rot in the water of the cell, kg/m3 (water.quality.v1, D2508); NaN when unanswered
    double   salt_kg_m3 = 0.0;   // dissolved salt in the water of the cell, kg/m3 (water.quality.v1, D2508); NaN when unanswered
    double   hue_deg = 0.0;      // hue the water of the cell shows the eye, degrees 0 to 360 (water.quality.v1, curr_hue_deg, CAUSA_ASE_SIGNATURE); NaN when unanswered
};

}  // namespace ase::adp::godot
