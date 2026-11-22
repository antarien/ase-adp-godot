#pragma once

/**
 * ASE GODOT ADAPTER - THE BEND OF A BARK TUBE PIECE
 *
 * @file        godot_tube_bend.hpp
 * @brief       bend_rings - where the intermediate rings of a bent piece stand and how their frames turn
 * @description PLAN_ASE_VIVARIUM_TREEGEN T.6, section "Zwischenringe nach Krümmung" (usecase:101643 ring the tubes
 *              by curvature): ase-render-data gives every piece a count of intermediate rings by the turn of its
 *              tangent between the lower knot and the tip knot (RenderDataCylCrvtCalcSystem), and the knot port
 *              carries it. Between the two knots runs the cubic Hermite curve through both centres, its end
 *              tangents the frame tangents of the knots (local y) scaled by the chord. Ring i of R stands on it at
 *              t = i / (R + 1); its frame is the frame below it, carried on by the least turn onto the tangent of
 *              the curve (parallel transport), so the corners of one ring stand over the corners of the ring below
 *              and the bark does not twist. Radius, v and sway share run linearly in t; tubes_by_body lays them.
 *
 *              WHY A UNIT OF ITS OWN: the curve is pure geometry of two knot views, read only by the tube layout;
 *              godot_plant_tubes.cpp lays rings and closes quads and stays one responsibility.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @category    render/pipeline/stage
 * @usecase     usecase:101643 - ring the tubes by curvature
 * @actor       Tube Ring Divider (TUBERINDIV)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/tube/godot_knot_resource_manager.hpp>

#include <cstdint>

namespace ase::adp::godot {

/** One intermediate ring of a piece: its centre, its carried frame and its place t along the piece, 0 < t < 1. */
struct TubeBendRing {
    float x = 0.0f;   // centre of the ring on the Hermite curve, metres east of the stem base
    float y = 0.0f;   // centre of the ring, metres above the ground at the stem base
    float z = 0.0f;   // centre of the ring, metres north of the stem base
    float qx = 0.0f;  // carried frame, vector part, east share: local y is the tangent of the curve
    float qy = 0.0f;  // carried frame, vector part, upward share
    float qz = 0.0f;  // carried frame, vector part, north share
    float qw = 1.0f;  // carried frame, scalar part
    float t = 0.0f;   // place along the piece, i / (rings + 1): radius, v and sway share run linearly in it
};

/**
 * Fills out[0 .. rings - 1] with the intermediate rings between the lower knot and the tip knot of one piece, in
 * the order they stand from the lower knot up. `rings` is the count the knot port carried, at most
 * GODOT_TUBE_RING_MAX; out holds at least that many entries.
 */
void bend_rings(const VivariumKnotView& lower, const VivariumKnotView& tip, uint32_t rings, TubeBendRing* out);

}  // namespace ase::adp::godot
