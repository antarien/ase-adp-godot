#pragma once

/**
 * ASE GODOT ADAPTER - HOST BARK VIEW
 *
 * @file        godot_host_bark_view.hpp
 * @brief       The vertices of the bark tubes of the embedded host, as the last read of the bark port showed them
 * @description The copy GodotHostResourceManager keeps of every vertex of the bark tube of an axis (register
 *              B0295 point 4, usecase:101613 show the plant bark tubes). One purpose of the dissolved
 *              godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header ratchet): only the units that
 *              build the bark tubes read it.
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
 * One vertex of the bark tube of an axis as the last read of the bark port showed it - a copy (register B0295
 * point 4, usecase:101613 show the plant bark tubes). ase-render-data built the tube, one knot ring per seam that
 * parent and child share, with frames carried by parallel transport; the view takes place, normal and texture
 * coordinates into the mesh of the plant and closes the quad strip between two rings by the links alone.
 */
struct VivariumBarkVertexView {
    uint32_t object_id = 0;   // the vertex row, as the plugin names it on its port
    uint32_t body = 0;        // object id of the patch the plant stands on
    double   x = 0.0;         // metres east of the stem base
    double   y = 0.0;         // metres above the ground at the stem base
    double   z = 0.0;         // metres north of the stem base
    double   nx = 0.0;        // unit normal, east share
    double   ny = 0.0;        // unit normal, upward share
    double   nz = 0.0;        // unit normal, north share
    double   u = 0.0;         // around the ring, 0..1 (the closing vertex 1)
    double   v = 0.0;         // along the axis from its foot ring, metres
    uint32_t next = 0;        // object id of the next vertex on the same ring, InvalidEntityId after the closing one
    uint32_t below = 0;       // object id of the vertex at the same place on the ring below, InvalidEntityId on a foot ring
    uint32_t organ = 0;       // object id of the internode whose knot ring the vertex belongs to
};

}  // namespace ase::adp::godot
