#pragma once

/**
 * ASE GODOT ADAPTER - HOST TEMPLATE VIEW
 *
 * @file        godot_host_template_view.hpp
 * @brief       The corners and triangles of the organ templates of the embedded host, as the last snapshot showed them
 * @description The copies GodotHostResourceManager keeps of the template corner port and the template triangle
 *              port (usecase:101011 tessellate the organ templates). One purpose of the dissolved
 *              godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header ratchet): only the units that
 *              build the template meshes read it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-21
 * @modified    2026-11-23
 * @version     00.00.04.00024 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * One corner of an organ template as the last snapshot of the template corner port showed it - a copy
 * (usecase:101011 tessellate the organ templates). ase-render-data builds the corner on the unit - base at the
 * origin, length 1 along +y - and the view takes its place, normal, texture coordinates and tangent into
 * the template mesh of its code and computes none of them. The tangent comes from the template tangent
 * port (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.6), joined to the corner of the same code and index.
 */
struct VivariumTemplateCornerView {
    uint32_t object_id = 0;   // the corner row, as the plugin names it on its port
    uint32_t code = 0;        // GODOT_TMPL_* of the template it belongs to
    uint32_t index = 0;       // its place in the template, named by the triangles
    double   x = 0.0;         // place on the unit, across
    double   y = 0.0;         // place on the unit, along the organ
    double   z = 0.0;         // place on the unit, across
    double   nx = 0.0;        // unit normal, x share
    double   ny = 0.0;        // unit normal, y share
    double   nz = 0.0;        // unit normal, z share
    double   u = 0.0;         // texture coordinate around the ring or across the leaf
    double   v = 0.0;         // texture coordinate along the organ
    double   tx = 0.0;        // unit tangent along growing u, x share - from the template tangent port, joined by code and index
    double   ty = 0.0;        // unit tangent, y share
    double   tz = 0.0;        // unit tangent, z share
    double   tw = 0.0;        // sign of the bitangent: +1 with growing v, -1 against it
};

/**
 * One triangle of an organ template as the last snapshot of the template triangle port showed it - a
 * copy. Its three corners are indices of the corners of the same code, clockwise seen from outside -
 * Godot's front face - in the order ase-render-data laid them; the view decides no winding.
 */
struct VivariumTemplateTriangleView {
    uint32_t object_id = 0;   // the triangle row, as the plugin names it on its port
    uint32_t code = 0;        // GODOT_TMPL_* of the template it belongs to
    uint32_t a = 0;           // index of its first corner
    uint32_t b = 0;           // index of its second corner
    uint32_t c = 0;           // index of its third corner
};

}  // namespace ase::adp::godot
