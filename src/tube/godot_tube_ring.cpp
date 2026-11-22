/**
 * ASE GODOT ADAPTER - THE RING OF A BARK TUBE WITH THE CORNERS OF ITS AXIS
 *
 * @file        godot_tube_ring.cpp
 * @brief       lay_tube_ring, lay_tube_disk, axis_foot, twig_stride - one ring of K corners and the foot of its axis
 * @description See godot_tube_ring.hpp.
 *
 *              THE RING IS THE ONE ase-render-data LAID: corner k stands at the angle 2 pi k / K in the frame of
 *              its knot (render_data_cyl_vtx_calc_sys.cpp), K the count of its axis. The bark shader turns the
 *              template direction by the frame on custom2 and sets it out by the radius on custom1, so the same K
 *              on both sides gives the same corners; the closing vertex K stands at the place of corner 0 with
 *              u = 1, so the texture closes without a seam. Every vertex carries the noise key of its axis on custom3;
 *              the shader spreads it to o_axis and displaces the corner along its normal by the bark noise.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @category    render/pipeline/stage
 * @usecase     usecase:101644 - count ring corners by size
 * @actor       Tube Ring Divider (TUBERINDIV)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/tube/godot_tube_ring.hpp>
#include <ase/adp/godot/tube/types.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/math/math.hpp>

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ase::adp::godot {

int32_t lay_tube_ring(PlantBodyTube& tube, double x, double y, double z, double radius, double qx, double qy, double qz,
                      double qw, double v, const VivariumOrganView& organ, float slot, double share,
                      const VivariumFootView& axis) {
    const uint32_t sides = axis.sides;
    const int32_t first = static_cast<int32_t>(tube.arrays.positions.size() / 3u);
    // v runs in metres, the tile record counts the tiles along the whole internode: per metre of it
    const double along = organ.length > 0.0 ? organ.tile_along / organ.length : organ.tile_along;
    for (uint32_t k = 0u; k <= sides; ++k) {
        const float around = static_cast<float>(k) / static_cast<float>(sides);
        const float angle = ase::math::TWO_PI * around;
        tube.arrays.positions.push_back(static_cast<float>(x));
        tube.arrays.positions.push_back(static_cast<float>(y));
        tube.arrays.positions.push_back(static_cast<float>(z));
        tube.arrays.normals.push_back(ase::math::cos(angle));
        tube.arrays.normals.push_back(0.0f);
        tube.arrays.normals.push_back(-ase::math::sin(angle));
        // the tangent of the normal map runs where u grows, the derivative of the template direction by the angle;
        // the normal crossed with it is the axis, where v grows, and Godot takes the binormal where v falls (its
        // CylinderMesh lays v downward with w = +1), so the sign of the binormal is GODOT_TUBE_BINORMAL_SIGN
        tube.tangents.push_back(-ase::math::sin(angle));
        tube.tangents.push_back(0.0f);
        tube.tangents.push_back(-ase::math::cos(angle));
        tube.tangents.push_back(GODOT_TUBE_BINORMAL_SIGN);
        tube.arrays.uvs.push_back(around);
        tube.arrays.uvs.push_back(static_cast<float>(v));
        tube.custom0.push_back(static_cast<float>(organ.bark));
        tube.custom0.push_back(static_cast<float>(organ.tile_around));
        tube.custom0.push_back(static_cast<float>(along));
        tube.custom0.push_back(static_cast<float>(organ.tile_base));
        tube.custom1.push_back(slot);
        tube.custom1.push_back(static_cast<float>(share));
        tube.custom1.push_back(static_cast<float>(radius));
        tube.custom1.push_back(static_cast<float>(organ.order));
        tube.custom2.push_back(static_cast<float>(qx));
        tube.custom2.push_back(static_cast<float>(qy));
        tube.custom2.push_back(static_cast<float>(qz));
        tube.custom2.push_back(static_cast<float>(qw));
        tube.custom3.push_back(static_cast<float>(axis.noise));
    }
    return first;
}

/**
 * The rim is the tip ring of the knot, its uv turned into the wood texture around GODOT_SCAR_UV_CENTRE; the centre
 * vertex stands at the knot with radius 0 and the axis as its template direction, and carries the bark, sway and
 * frame of the rim. The fan (centre, k + 1, k) faces along the axis, out of the stub, as Godot takes a clockwise
 * triangle as its front.
 */
int32_t lay_tube_disk(PlantBodyTube& tube, const VivariumKnotView& knot, const VivariumOrganView& organ, float slot,
                      const VivariumFootView& axis) {
    const uint32_t sides = axis.sides;
    const int32_t rim = lay_tube_ring(tube, knot.x, knot.y, knot.z, knot.radius, knot.qx, knot.qy, knot.qz, knot.qw,
                                      knot.v, organ, slot, organ.sway_top, axis);
    const std::size_t rim_at = static_cast<std::size_t>(rim);
    for (uint32_t k = 0u; k <= sides; ++k) {
        const float angle = ase::math::TWO_PI * static_cast<float>(k) / static_cast<float>(sides);
        tube.arrays.uvs[(rim_at + k) * 2u] = GODOT_SCAR_UV_CENTRE + GODOT_SCAR_UV_HALF * ase::math::cos(angle);
        tube.arrays.uvs[(rim_at + k) * 2u + 1u] = GODOT_SCAR_UV_CENTRE - GODOT_SCAR_UV_HALF * ase::math::sin(angle);
    }
    const int32_t centre = static_cast<int32_t>(tube.arrays.positions.size() / 3u);
    tube.arrays.positions.push_back(static_cast<float>(knot.x));
    tube.arrays.positions.push_back(static_cast<float>(knot.y));
    tube.arrays.positions.push_back(static_cast<float>(knot.z));
    // the axis of the template: local y, the tangent of the knot frame
    tube.arrays.normals.push_back(0.0f);
    tube.arrays.normals.push_back(1.0f);
    tube.arrays.normals.push_back(0.0f);
    // u of the wood grows with local x, the direction of the first rim corner
    tube.tangents.push_back(1.0f);
    tube.tangents.push_back(0.0f);
    tube.tangents.push_back(0.0f);
    tube.tangents.push_back(GODOT_TUBE_BINORMAL_SIGN);
    tube.arrays.uvs.push_back(GODOT_SCAR_UV_CENTRE);
    tube.arrays.uvs.push_back(GODOT_SCAR_UV_CENTRE);
    // bark, sway and frame as the rim carries them, the radius 0: the centre stays on the knot
    for (uint32_t i = 0u; i < GODOT_BARK_CUSTOM_VALUES; ++i) {
        tube.custom0.push_back(tube.custom0[rim_at * GODOT_BARK_CUSTOM_VALUES + i]);
        tube.custom1.push_back(tube.custom1[rim_at * GODOT_BARK_CUSTOM_VALUES + i]);
    }
    tube.custom1[static_cast<std::size_t>(centre) * GODOT_BARK_CUSTOM_VALUES + 2u] = 0.0f;
    for (uint32_t i = 0u; i < GODOT_TUBE_FRAME_VALUES; ++i) {
        tube.custom2.push_back(tube.custom2[rim_at * GODOT_TUBE_FRAME_VALUES + i]);
    }
    for (uint32_t i = 0u; i < GODOT_TUBE_NOISE_VALUES; ++i) {
        tube.custom3.push_back(tube.custom3[rim_at * GODOT_TUBE_NOISE_VALUES + i]);
    }
    for (int32_t k = 0; k < static_cast<int32_t>(sides); ++k) {
        tube.scar_indices.push_back(centre);
        tube.scar_indices.push_back(rim + k + 1);
        tube.scar_indices.push_back(rim + k);
    }
    return centre;
}

/**
 * ase-render-data counts K once at the axis foot and lays every ring of the axis with it, and the foot record carries
 * the noise key of the axis beside it, so the walk ends at the first knot whose value is known or at the knot whose
 * `below` names itself, whose foot holds the value. The walk holds at most as many steps as there are knots, so a
 * broken chain that closes on itself ends with 0.
 */
uint32_t axis_foot(uint32_t object_id, const std::unordered_map<uint32_t, const VivariumKnotView*>& knot_by_id,
                   const std::unordered_map<uint32_t, uint32_t>& foot_value,
                   std::unordered_map<uint32_t, uint32_t>& known) {
    std::vector<uint32_t> walked;
    uint32_t at = object_id;
    uint32_t value = 0u;
    while (walked.size() <= knot_by_id.size()) {
        const auto seen = known.find(at);
        if (seen != known.end()) {
            value = seen->second;
            break;
        }
        const auto knot = knot_by_id.find(at);
        if (knot == knot_by_id.end()) {
            break;
        }
        walked.push_back(at);
        if (knot->second->below == at) {
            const auto foot = foot_value.find(at);
            value = foot != foot_value.end() ? foot->second : 0u;
            break;
        }
        at = knot->second->below;
    }
    for (const uint32_t id : walked) {
        known[id] = value;
    }
    return value;
}

uint32_t twig_stride(uint32_t sides) {
    uint32_t stride = 0u;
    for (uint32_t divisor = 1u; divisor <= sides; ++divisor) {
        if (sides % divisor == 0u && sides / divisor >= GODOT_TWIG_LEAST_CORNERS) {
            stride = divisor;
        }
    }
    return stride;
}

}  // namespace ase::adp::godot
