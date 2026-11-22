#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM HUD LOOSE BRIDGE
 *
 * @file        godot_vivarium_hud_loose.hpp
 * @brief       The loosened soil point by point the bridge keeps while flora.patch.v1 carries no loose soil
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD bridge section, design decision 2026-11-10: the HUD draws every
 *              element from its frame and never asks whether a port delivered. The loosened soil (N0456,
 *              flora.patch.v1 loose) is the mockup's LO while that port is silent, every rule a
 *              hud_bridge_loose_* token of SHA (theme.css --ase-hud-bridge-loose-*).
 *
 *              THE GRID. The mockup keeps one share 0..1 per point of its ground grid of 46 by 35 steps. The
 *              bridge lays that grid on the ground square of the start configuration from its south-west corner,
 *              hud_bridge_loose_cell_x_m east and hud_bridge_loose_cell_z_m north between two points (the
 *              mockup's own reading of a share as metres), every point at the start share
 *              hud_bridge_patch_loose_share.
 *
 *              A TAP OF A LOOSEN VERB (applyTerr, m.html 980). Every point within the verb's brush gains
 *              hud_bridge_loose_gain * (1 - (d / brush)^2), at most 1. Over living roots it costs the growth
 *              hud_bridge_loose_root_cost (the caller takes it, S.grow - 4 of 100).
 *
 *              RAIN SETTLES IT (the mockup's tick, m.html 1030). Each game hour of rain multiplies every share by
 *              1 - hours * hud_bridge_loose_settle_rain, of drizzle by 1 - hours * hud_bridge_loose_settle_drizzle,
 *              never below 0; the hours are the clock's since the frame before, the first frame only notes it.
 *
 *              THE PATCH'S SHARE (patchLoose, m.html 372). The mean of the points within hud_bridge_loose_patch_m
 *              of the patch's centre; a centre without a point in reach keeps the start share.
 *
 *              THE GAP THIS BRIDGE HOLDS (D2473 and D2475 under N0456, carrier N0448). The grid above is a raster
 *              beside ase-gis and the settling a computation of soil state in the adapter; both stand only while no
 *              owner writes the soil. The raster layer is designed, not built: CAUSA_ASE_GIS names
 *              LYR_LITHO_COMPACTION (soil compaction 0 to 1) among the soil layers, and ase-gis carries no
 *              GIS_LYR_LTH_ identifier for it (measured 2026-11-20). GisStatLthSoilPrflWorkComponent::compaction is
 *              a field of a soil profile, not a cell of a raster, and has no writer in modules, plugins or adapter.
 *              The way: the owner of the soil writes compaction per cell of that layer at 1 Hz, only changed cells;
 *              the loosen verb is a request to that owner; rain settles it from GIS_LYR_ATMO_PRCP_MM_H at the
 *              owner; the patch reads the looseness of its cell through flora.patch.v1 loose. Then this class goes,
 *              migrated to that owner.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100389 - level the ground
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0456 (flora.patch.v1 loose), carrier N0448
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_start_config.hpp>
#include <ase/adp/godot/godot_hud_ground_crumbs.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** The mockup's LO on the ground square. */
class VivariumLooseBridge {
public:
    /** Reads the hud_bridge_loose_* rules and lays the grid on `square`; false with the missing name in `failed`. */
    bool load(const UiTheme& theme, const VivariumGroundSpec& square, double start_share, std::string& failed);

    /** A tap of a loosen verb at `x`, `z` (world metres) under a brush of `brush_m`. */
    void loosen(double x, double z, double brush_m);

    /** The rain of `weather` (a GODOT_HUD_WEATHER_* key, nullptr unknown) over the hours since the last call. */
    void settle(double game_hour, const char* weather);

    /** The mean share within the patch reach of `x`, `z`. */
    [[nodiscard]] double share(double x, double z) const;

    /** What a tap over living roots costs the growth, a share. */
    [[nodiscard]] double root_cost() const;

    /** Writes the grid into the crumbs' frame. */
    void frame(GroundCrumbsFrame& out) const;

private:
    std::vector<double> loose_;    // the share per point, row * columns_ + column
    int64_t columns_ = 0;          // points east
    int64_t rows_ = 0;             // points north
    double west_x_ = 0.0;          // the south-west point, world metres east
    double south_z_ = 0.0;         // the south-west point, world metres north
    double cell_x_ = 0.0;          // hud_bridge_loose_cell_x_m
    double cell_z_ = 0.0;          // hud_bridge_loose_cell_z_m
    double gain_ = 0.0;            // hud_bridge_loose_gain
    double root_cost_ = 0.0;       // hud_bridge_loose_root_cost
    double settle_rain_ = 0.0;     // hud_bridge_loose_settle_rain
    double settle_drizzle_ = 0.0;  // hud_bridge_loose_settle_drizzle
    double patch_m_ = 0.0;         // hud_bridge_loose_patch_m
    double start_ = 0.0;           // hud_bridge_patch_loose_share: every point's share before the first tap
    double hour_ = -1.0;           // the game hour of the last settle; below 0 before the first
};

}  // namespace ase::adp::godot
