#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the constants of the bark tubes the view lays from the knot and foot ports
 * @description All compile-time constants of the tube sub-type; NO runtime values - those belong in the views.
 *              PLAN_ASE_VIVARIUM_TREEGEN T.6: one record per internode crosses the knot port of ase-pl-flora
 *              (flora.mesh.knot.v1), one per internode that starts an axis the foot port (flora.mesh.foot.v1), and
 *              no vertex. The view lays the ring template around every knot and the bark shader turns it by the
 *              knot frame and scales it by the radius. These are the numbers of that template and of the paging of
 *              the two ports.
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter types.hpp is included by more than a hundred own units of
 *              the avt build and takes no new line (header ratchet, operator decision 2026-11-21); these values are
 *              read only by the knot reader and the tube layout, so they stand in the narrowest header those include.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:101613 - show the plant bark tubes
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: none - the tube sub-type names no file part of its own.
 */

constexpr uint32_t GODOT_KNOT_ROWS_PER_PASS = 4096u;          // knot rows ase-pl-flora files per pass on both ports (FLRA_HOST_KNOT_ROWS_PER_PASS): a new set takes one pass per page before the host swaps it in
constexpr const char* GODOT_KNOT_SCHEDULE = "Production";     // schedule in which ase-pl-flora files the knot and foot pages, one pass per tick (FloraHostCylNodeExptSystem)
constexpr uint32_t GODOT_TUBE_SIDE_MIN = 3u;                  // fewest corners K of a ring (RNDR_CYL_SIDE_MIN of ase-render-data, usecase:101644 - count ring corners by size): a foot record below it is a broken foot port
constexpr uint32_t GODOT_TUBE_SIDE_MAX = 16u;                 // most corners K of a ring (RNDR_CYL_SIDE_MAX of ase-render-data): a foot record above it is a broken foot port; every ring of an axis has K + 1 vertices, the closing one at the place of the first with u = 1
constexpr uint32_t GODOT_TUBE_RING_MAX = (GODOT_TUBE_SIDE_MAX + 1u) / 2u;  // most intermediate rings between two knots, ceil(K / 2) at the most corners: RenderDataCylCrvtCalcSystem bounds them by half a turn at the step 2 pi / K (usecase:101643 - ring the tubes by curvature); a count above it is a broken knot port
constexpr const char* GODOT_KEY_SIDES = "sides";             // foot record: the corners K of every ring of the axis it starts (FLORA_HOST_KEY_SIDES of ase-pl-flora)
constexpr const char* GODOT_KEY_NOISE = "noise";             // foot record: the key of o_axis, the place of the axis in the bark noise field (FLORA_HOST_KEY_NOISE of ase-pl-flora), the twelfth value
constexpr double GODOT_TUBE_NOISE_LEAST = 0.0;                // least noise key of a foot record: RenderDataCylOfstCalcSystem draws it in [0, 1) (RNDR_CYL_NOISE_SEED)
constexpr double GODOT_TUBE_NOISE_MOST = 1.0;                 // most noise key of a foot record: a key above it is a broken foot port
constexpr uint32_t GODOT_TUBE_NOISE_VALUES = 1u;             // values of the noise key beside every ring vertex of the knot form (CUSTOM3, R float): plant_bark.gdshader spreads it to o_axis
constexpr const char* GODOT_BARK_KNOT_FORM = "knot_form";    // uniform of the bark shader: true where the tube carries the knot frames on CUSTOM2, so the shader turns and sets out every corner
constexpr const char* GODOT_BARK_WOBBLE_AMP = "wobble_amp";  // uniform of the bark shader: bark_wobble_amp, the share of the ring radius the bark noise displaces a corner by at most
constexpr float GODOT_BARK_WOBBLE_MOST = 0.15f;               // bark_wobble_amp: PLAN_ASE_VIVARIUM_TREEGEN T.6 "Rindenrauschen" bounds the displacement at 0.15 * radius, and the shader holds its fractal sum in [-1, 1]
constexpr const char* GODOT_KEY_RINGS = "rings";             // knot record: intermediate rings between the lower ring and the tip knot (FLORA_HOST_KEY_RINGS of ase-pl-flora)
constexpr float GODOT_TUBE_TURN_REVERSE = 1.0e-6f;            // usecase:101643: a dot of two tangents closer than this to -1 turns them apart, the frame then turns half about its own across axis
constexpr float GODOT_TUBE_CURVE_STILL = 1.0e-6f;             // usecase:101643: a derivative of the Hermite curve shorter than this stands still, the tangent below is carried on
constexpr uint32_t GODOT_TUBE_FRAME_VALUES = 4u;             // values of the knot frame beside every ring vertex (CUSTOM2, RGBA float): qx, qy, qz, qw of the unit quaternion
constexpr float GODOT_TUBE_BINORMAL_SIGN = -1.0f;             // w of every ring tangent: normal x tangent is the axis, where v grows, and Godot takes the binormal where v falls (its CylinderMesh lays v downward with w = +1)
constexpr float GODOT_SCAR_UV_CENTRE = 0.5f;                  // uv of the centre of the wood disk on a cut face: the pith stands in the middle of the wood texture
constexpr float GODOT_SCAR_UV_HALF = 0.5f;                    // uv from the centre to the rim of the wood disk: the rim ring touches the edge of the wood texture, so the year rings fill the disk

}  // namespace ase::adp::godot
