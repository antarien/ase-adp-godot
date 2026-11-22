/**
 * ASE GODOT ADAPTER - THE BEND OF A BARK TUBE PIECE
 *
 * @file        godot_tube_bend.cpp
 * @brief       bend_rings - the intermediate rings of a bent piece on the Hermite curve between its two knots
 * @description See godot_tube_bend.hpp.
 *
 *              THE FRAME IS CARRIED, NEVER BUILT ANEW: each ring takes the frame of the ring below and turns it by
 *              the least turn from the tangent below onto its own. A frame built from the tangent alone would pick
 *              its across axis anew on every ring and twist the bark between them.
 *
 *              TWO TANGENTS THAT POINT APART have no least turn: every axis at right angles to both serves. The
 *              across axis of the frame below is one of them, so the frame turns half about it and its corners
 *              stay on their side of the tube.
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

#include <ase/adp/godot/tube/godot_tube_bend.hpp>
#include <ase/adp/godot/tube/types.hpp>
#include <ase/math/math.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/** Local y of a knot frame, the tangent of the axis at the knot: the unit quaternion turns (0, 1, 0). */
void frame_tangent(const float q[4], float out[3]) {
    out[0] = 2.0f * (q[0] * q[1] - q[3] * q[2]);
    out[1] = 1.0f - 2.0f * (q[0] * q[0] + q[2] * q[2]);
    out[2] = 2.0f * (q[1] * q[2] + q[3] * q[0]);
}

/** Local x of a knot frame, the across direction of corner 0: the unit quaternion turns (1, 0, 0). */
void frame_across(const float q[4], float out[3]) {
    out[0] = 1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]);
    out[1] = 2.0f * (q[3] * q[2] + q[0] * q[1]);
    out[2] = 2.0f * (q[0] * q[2] - q[3] * q[1]);
}

/**
 * Carries the frame q (x, y, z, w) from the unit tangent `from` onto the unit tangent `to` by the least turn between
 * them; two tangents that point apart turn half about the across axis of the frame.
 */
void carry_frame(float q[4], const float from[3], const float to[3]) {
    const float dot = from[0] * to[0] + from[1] * to[1] + from[2] * to[2];
    float turn[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    if (dot <= GODOT_TUBE_TURN_REVERSE - 1.0f) {
        frame_across(q, turn);
    } else {
        const float s = ase::math::sqrt(2.0f * (1.0f + dot));
        turn[0] = (from[1] * to[2] - from[2] * to[1]) / s;
        turn[1] = (from[2] * to[0] - from[0] * to[2]) / s;
        turn[2] = (from[0] * to[1] - from[1] * to[0]) / s;
        turn[3] = 0.5f * s;
    }
    // the turn first, then the frame: turn * q
    const float x = turn[3] * q[0] + turn[0] * q[3] + turn[1] * q[2] - turn[2] * q[1];
    const float y = turn[3] * q[1] - turn[0] * q[2] + turn[1] * q[3] + turn[2] * q[0];
    const float z = turn[3] * q[2] + turn[0] * q[1] - turn[1] * q[0] + turn[2] * q[3];
    const float w = turn[3] * q[3] - turn[0] * q[0] - turn[1] * q[1] - turn[2] * q[2];
    const float length = ase::math::sqrt(x * x + y * y + z * z + w * w);
    q[0] = x / length;
    q[1] = y / length;
    q[2] = z / length;
    q[3] = w / length;
}

}  // namespace

void bend_rings(const VivariumKnotView& lower, const VivariumKnotView& tip, uint32_t rings, TubeBendRing* out) {
    const float p0[3] = {static_cast<float>(lower.x), static_cast<float>(lower.y), static_cast<float>(lower.z)};
    const float p1[3] = {static_cast<float>(tip.x), static_cast<float>(tip.y), static_cast<float>(tip.z)};
    const float chord = ase::math::sqrt((p1[0] - p0[0]) * (p1[0] - p0[0]) + (p1[1] - p0[1]) * (p1[1] - p0[1]) +
                                        (p1[2] - p0[2]) * (p1[2] - p0[2]));
    float q[4] = {static_cast<float>(lower.qx), static_cast<float>(lower.qy), static_cast<float>(lower.qz),
                  static_cast<float>(lower.qw)};
    const float q1[4] = {static_cast<float>(tip.qx), static_cast<float>(tip.qy), static_cast<float>(tip.qz),
                         static_cast<float>(tip.qw)};
    float t0[3];
    float t1[3];
    frame_tangent(q, t0);
    frame_tangent(q1, t1);
    float carried[3] = {t0[0], t0[1], t0[2]};
    const float span = static_cast<float>(rings + 1u);
    for (uint32_t i = 0u; i < rings; ++i) {
        const float t = static_cast<float>(i + 1u) / span;
        const float tt = t * t;
        const float ttt = tt * t;
        // the cubic Hermite basis and its derivative by t
        const float h00 = 2.0f * ttt - 3.0f * tt + 1.0f;
        const float h10 = ttt - 2.0f * tt + t;
        const float h01 = 3.0f * tt - 2.0f * ttt;
        const float h11 = ttt - tt;
        const float d00 = 6.0f * tt - 6.0f * t;
        const float d10 = 3.0f * tt - 4.0f * t + 1.0f;
        const float d01 = 6.0f * t - 6.0f * tt;
        const float d11 = 3.0f * tt - 2.0f * t;
        float at[3];
        float along[3];
        for (uint32_t axis = 0u; axis < 3u; ++axis) {
            at[axis] = h00 * p0[axis] + h10 * chord * t0[axis] + h01 * p1[axis] + h11 * chord * t1[axis];
            along[axis] = d00 * p0[axis] + d10 * chord * t0[axis] + d01 * p1[axis] + d11 * chord * t1[axis];
        }
        const float speed = ase::math::sqrt(along[0] * along[0] + along[1] * along[1] + along[2] * along[2]);
        if (speed > GODOT_TUBE_CURVE_STILL) {
            const float to[3] = {along[0] / speed, along[1] / speed, along[2] / speed};
            carry_frame(q, carried, to);
            carried[0] = to[0];
            carried[1] = to[1];
            carried[2] = to[2];
        }
        TubeBendRing& ring = out[i];
        ring.x = at[0];
        ring.y = at[1];
        ring.z = at[2];
        ring.qx = q[0];
        ring.qy = q[1];
        ring.qz = q[2];
        ring.qw = q[3];
        ring.t = t;
    }
}

}  // namespace ase::adp::godot
