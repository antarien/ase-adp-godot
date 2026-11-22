#pragma once

/**
 * ASE GODOT ADAPTER - HOST STREAM VIEW
 *
 * @file        godot_host_stream_view.hpp
 * @brief       The water of the brooks of the embedded host, as the last read of the stream port showed it
 * @description The copy GodotHostResourceManager keeps of every brook with water: the course of its bed, the
 *              water ase-hydro lets run in it and the groundwater beside it. One purpose of the dissolved
 *              godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header ratchet): only the units that
 *              draw or read the brook water read it.
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
 * One brook with water as the last read of the stream port showed it (usecase:100399 carry brook water): the
 * course of its bed and the water ase-hydro lets run in it. A bed without measured water has no record. The
 * groundwater beside the bed (B0286) is the one optional pair of the record: until ase-hydro publishes it the
 * port carries neither key, both fields stay NaN, and the view lays no water beside the bed.
 */
struct VivariumStreamView {
    uint32_t object_id = 0;       // the brook, as the plugin names it on its port
    double   x = 0.0;             // start of the bed, world metres east
    double   z = 0.0;             // start of the bed, world metres north
    double   end_x = 0.0;         // end of the bed, world metres east
    double   end_z = 0.0;         // end of the bed, world metres north
    double   half_m = 0.0;        // half width of the bed, metres
    double   depth_m = 0.0;       // water depth, metres
    double   velocity_m_s = 0.0;  // mean flow velocity, metres per second
    double   discharge_m3_s = 0.0;  // discharge, cubic metres per second
    double   groundwater_level_m = 0.0;  // groundwater table beside the bed, world metres; NaN while the port carries none (N0484)
    double   groundwater_reach_m = 0.0;  // distance from the bed's line over which that table holds, metres; NaN likewise
};

}  // namespace ase::adp::godot
