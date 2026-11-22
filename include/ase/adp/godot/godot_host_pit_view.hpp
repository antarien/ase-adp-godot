#pragma once

/**
 * ASE GODOT ADAPTER - HOST PIT VIEW
 *
 * @file        godot_host_pit_view.hpp
 * @brief       The open water of every dug pit of the embedded host, as the last read of the pit port showed it
 * @description The copy GodotHostResourceManager keeps of every pit ase-pl-hydrogeology lays on hydrogeology.pit.v1
 *              (HydrogeologyHostPitExptSystem): the centre of the soil cell of the pit, the open water over its floor
 *              and the water running over its lowest rim. One purpose of its own (WRFL_ASE_TIER_WIPE, the header
 *              ratchet): only the units that draw or read the pit water include it.
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
 * One dug pit as the last read of the pit port showed it (usecase:100399 carry brook water, computed by ase-hydro
 * HydroCellPitCalcSystem and answered by the ase-gis cell). The water surface of the pit stands depth_m over the
 * floor the ground of the view already carries, since ase-terrain dug it; the view lays it there and computes none
 * of these values. A value the cell never answered is missing from the record, and its field stays NaN.
 */
struct VivariumPitView {
    uint32_t object_id = 0;     // the pit, its HUB_HYDR_PIT row as the plugin names it on its port
    double   x = 0.0;           // centre of the soil cell of the pit, world metres east (pit_x)
    double   z = 0.0;           // centre of the soil cell of the pit, world metres north (pit_z)
    double   depth_m = 0.0;     // open water over the pit floor, metres; NaN when unanswered
    double   spill_m3_s = 0.0;  // water running over the lowest rim, cubic metres per second; NaN when unanswered
};

}  // namespace ase::adp::godot
