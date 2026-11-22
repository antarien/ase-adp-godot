#pragma once

/**
 * ASE GODOT ADAPTER - HOST TERRAIN VIEW
 *
 * @file        godot_host_terrain_view.hpp
 * @brief       The terrain samples and ground cells of the embedded host, as the last snapshot showed them
 * @description The copies GodotHostResourceManager keeps of the terrain port and the ground cell port: the
 *              vertices of the ground and the cells that join four of them. One purpose of the dissolved
 *              godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header ratchet): only the units that
 *              build or sample the ground read it.
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
 * One terrain sample as the last snapshot of the terrain port showed it - a copy: a vertex of the
 * ground. Place and height are ase-terrain's, normal and texture coordinates ase-render-data's; the
 * view takes all of them into the ground mesh and computes none of its own.
 */
struct VivariumTerrainView {
    uint32_t object_id = 0;   // the sample, as the plugin names it on its port
    uint32_t body = 0;        // the ground the sample belongs to
    double   x = 0.0;         // world metres east
    double   z = 0.0;         // world metres north
    double   height = 0.0;    // ground height, metres
    double   nx = 0.0;        // unit normal of the ground, east share
    double   ny = 0.0;        // unit normal of the ground, upward share
    double   nz = 0.0;        // unit normal of the ground, north share
    double   u = 0.0;         // texture coordinate, world metres east
    double   v = 0.0;         // texture coordinate, world metres north
};

/**
 * One ground cell as the last snapshot of the ground cell port showed it - a copy. The four corners
 * name terrain samples by their object id, in the order of the index buffer: the view draws the
 * triangles (sw, se, ne) and (sw, ne, nw), Godot's front face looking up (ase-render-data,
 * RenderDataGrndCellCalcSystem).
 */
struct VivariumGroundCellView {
    uint32_t object_id = 0;   // the cell's south-west sample, as the plugin names it on its port
    uint32_t body = 0;        // the ground the cell belongs to
    uint32_t sw = 0;          // object id of the south-west corner sample
    uint32_t se = 0;          // object id of the south-east corner sample
    uint32_t ne = 0;          // object id of the north-east corner sample
    uint32_t nw = 0;          // object id of the north-west corner sample
};

}  // namespace ase::adp::godot
