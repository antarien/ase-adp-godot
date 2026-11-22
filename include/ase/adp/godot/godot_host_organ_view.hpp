#pragma once

/**
 * ASE GODOT ADAPTER - HOST ORGAN VIEW
 *
 * @file        godot_host_organ_view.hpp
 * @brief       The organs of the plants of the embedded host and their instances, as the last snapshot showed them
 * @description The copies GodotHostResourceManager keeps of the organ ports and the instance ports: the place,
 *              frame, size, bark, tiling and sway of every organ, and the plant-local 3x4 of every drawn one.
 *              One purpose of the dissolved godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header
 *              ratchet): only the units that draw or read the organs read it.
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

#include <ase/adp/godot/leaf/types.hpp>
#include <ase/adp/godot/port/types.hpp>

#include <cstdint>

namespace ase::adp::godot {

/**
 * One organ as the last snapshot of its port showed it - a copy, never a reference into the App. The
 * place is plant-local (origin at the stem base on the ground, y up, x east, z north); body names the
 * patch the view stands the plant on.
 *
 * Every organ carries its frame in place of a direction: the unit quaternion ase-render-data built
 * from the roll of ase-phytomer (shoot) or ase-radix (root). It turns the plant frame into the organ's -
 * local y along the organ, local x along its roll axis.
 */
struct VivariumOrganView {
    uint32_t object_id = 0;   // the organ, as the plugin names it on its port
    uint32_t body = 0;        // object id of the patch the plant stands on
    double   order = 0.0;     // branching order of its axis (stem 0)
    double   x = 0.0;         // metres east of the stem base
    double   y = 0.0;         // metres above the ground at the stem base
    double   z = 0.0;         // metres north of the stem base
    double   qx = 0.0;        // frame, vector part, east share
    double   qy = 0.0;        // frame, vector part, upward share
    double   qz = 0.0;        // frame, vector part, north share
    double   qw = 0.0;        // frame, scalar part
    double   length = 0.0;    // metres (leaf: edge of the equal square; catkin and fruit stand: hang)
    double   radius = 0.0;    // metres (internode and root segment; else 0)
    double   bark = 0.0;      // share of the stem-base bark at its middle, 0..1 (internode, GODOT_KEY_BARK; else 0)
    double   tile_around = GODOT_TILE_NONE;  // tiles of the white bark around the tube (internode, tile port; else none)
    double   tile_along = GODOT_TILE_NONE;   // tiles of the white bark along the tube (internode, tile port; else none)
    double   tile_base = GODOT_TILE_NONE;    // tiles of the stem-base bark per white tile (internode, tile port; else none)
    double   sway_freq = GODOT_SWAY_NONE;    // natural frequency of its branch, Hz (sway port; a root organ none)
    double   sway_mean_x = GODOT_SWAY_NONE;  // east mean displacement of the tip of its branch, metres (sway port)
    double   sway_mean_z = GODOT_SWAY_NONE;  // north mean displacement of the tip of its branch, metres (sway port)
    double   sway_gust_x = GODOT_SWAY_NONE;  // east gust amplitude at the tip of its branch, metres (sway port)
    double   sway_gust_z = GODOT_SWAY_NONE;  // north gust amplitude at the tip of its branch, metres (sway port)
    double   sway_base = GODOT_SWAY_NONE;    // share of the tip displacement at its base (sway port)
    double   sway_top = GODOT_SWAY_NONE;     // share of the tip displacement at its top (sway port)
    double   bend_across = GODOT_BEND_NONE;  // lift of the blade edge over the midrib, leaf lengths (leaf, bend port; else flat)
    double   bend_along = GODOT_BEND_NONE;   // lift of the blade tip over its foot, leaf lengths (leaf, bend port; else flat)
    double   tint_offset = GODOT_TINT_NONE;  // colour offset of the card, laid on its albedo as 1 + offset (leaf, tint port; else the colour of its species)
    double   atlas_cell = GODOT_ATLS_CELL_NONE;    // cell of the leaf texture the card shows, below atlas_cells (leaf, atlas port; else the only cell)
    double   atlas_cells = GODOT_ATLS_CELLS_NONE;  // cells of the leaf texture of its species along u (leaf, atlas port; else the whole texture)
};

/**
 * One organ's instance as the last snapshot of its instance port showed it - a copy. The twelve values
 * are the plant-local 3x4 ase-render-data computed, in the row order of MultiMesh::set_buffer for
 * TRANSFORM_3D; the view hands them over unchanged and stands the plant at its patch as a scene node.
 * The organ record of the same object id names body and order.
 */
struct VivariumInstanceView {
    uint32_t object_id = 0;                    // the organ, as the plugin names it on its port
    float    rows[GODOT_INSTANCE_VALUES] = {};  // row 0 (east), row 1 (up), row 2 (north): x, y, z axis and origin
};

}  // namespace ase::adp::godot
