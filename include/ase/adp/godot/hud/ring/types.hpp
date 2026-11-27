#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the ring, the text anchors and the hexagon of the HUD drawing
 * @description The ring of the HUD (06.3 UiRing): the mockup's arcP measures an angle in degrees from the top
 *              clockwise, closes a full turn just short of 360 so that the path stays an arc, and drops a sweep
 *              below a tenth of a degree; the text anchors and the raster a HUD line is shaped at, and the corners
 *              of a hexagon chip.
 *
 *              WHY A HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated by
 *              purpose and stays a pure collector. These shapes are read by the pen of the HUD.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-25
 * @modified    2026-11-25
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

constexpr double GODOT_RING_TOP_DEG = 90.0;                         // arcP: (a - 90) degrees turns the mockup's top to Godot's angle 0 at +x
constexpr double GODOT_RING_FULL_DEG = 359.9;                       // arcP: degrees a full-turn sweep is cut to, so the path stays one arc
constexpr double GODOT_RING_MIN_DEG = 0.1;                          // ARC: degrees below which a sweep draws no path
constexpr double GODOT_RING_TURN_DEG = 360.0;                       // degrees of one turn
constexpr double GODOT_RING_POINTS_PER_TURN = 96.0;                 // polyline points draw_arc gets for a full circle; a shorter arc gets its share
constexpr int32_t GODOT_RING_POINTS_MIN = 3;                        // fewest polyline points of the shortest arc
constexpr double GODOT_TEXT_ANCHOR_START = 0.0;                     // text-anchor start: the whole width right of x
constexpr double GODOT_TEXT_ANCHOR_MIDDLE = 0.5;                    // text-anchor middle: half the width left of x
constexpr double GODOT_TEXT_ANCHOR_END = 1.0;                       // text-anchor end: the whole width left of x
constexpr int64_t GODOT_TEXT_RASTER_MIN = 1;                        // smallest whole raster size a HUD line is shaped at
constexpr int32_t GODOT_HEX_CORNERS = 6;                            // corners of a HUD hexagon chip, the first at 0 degrees

}  // namespace ase::adp::godot
