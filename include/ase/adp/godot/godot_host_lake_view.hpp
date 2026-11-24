#pragma once

/**
 * ASE GODOT ADAPTER - HOST LAKE VIEW
 *
 * @file        godot_host_lake_view.hpp
 * @brief       The still water of the lakes and ponds of the embedded host, as the last read of the lake port showed it
 * @description The copy the water kinds reader keeps of every lake or pond body ase-pl-water lays on water.lake.v1:
 *              the axis-aligned box of the body (SPT_AABB_*, written by TerrainHubPubHydSystem), its depth and its
 *              kind. The box is in the frame of the engine: x east, y north, z the height, so the surface of the body
 *              is the top face of the box, centre z plus half z. One purpose of its own (WRFL_ASE_TIER_WIPE, the
 *              header ratchet): only the units that draw or read the lakes include it.
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
 * One lake or pond body as the last read of the lake port showed it. The plugin lays every value the body answered;
 * a value never answered stays NaN. The view computes none of them.
 */
struct VivariumLakeView {
    uint32_t object_id = 0;  // the body, its row as the plugin names it on its port
    double   cx = 0.0;       // centre of the box, metres east (lake_cx, SPT_AABB_CX); NaN when unanswered
    double   cy = 0.0;       // centre of the box, metres north (lake_cy, SPT_AABB_CY); NaN when unanswered
    double   cz = 0.0;       // centre of the box, metres of height above NN0, halfway between floor and surface (lake_cz, SPT_AABB_CZ); NaN when unanswered
    double   hx = 0.0;       // half extent of the box east to west, metres (lake_hx, SPT_AABB_HX); NaN when unanswered
    double   hy = 0.0;       // half extent of the box north to south, metres (lake_hy, SPT_AABB_HY); NaN when unanswered
    double   hz = 0.0;       // half the span between floor and surface, metres (lake_hz, SPT_AABB_HZ); NaN when unanswered
    double   depth_m = 0.0;  // depth of the water, metres (lake_depth_m); NaN when unanswered
    double   kind = 0.0;     // the code of the body, lake or pond (lake_kind); NaN when unanswered
};

}  // namespace ase::adp::godot
