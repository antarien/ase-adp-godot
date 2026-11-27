#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the tile edge and the no-place mark of the sight line over the ground
 * @description The sight line over the drawn ground to the water of a brook (usecase:101650 show the river water,
 *              P0733): GroundTops keeps the highest sample of every tile of GODOT_GROUND_TILE_CELLS by
 *              GODOT_GROUND_TILE_CELLS cells, and the line from the eye passes over a tile whose highest sample stays
 *              under it without asking a place in it.
 *
 *              WHY A TYPES HEADER OF ITS OWN: a constant goes into the narrowest header its readers include (header
 *              ratchet, operator decision 2026-11-21); the adapter hud/types.hpp is a collector of its purpose
 *              headers and takes no line of its own. These two names are read by GroundTops and its test only.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:101650 - show the river water
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
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
 * ABBREVIATIONS: tile = a square of ground cells with one highest sample; place = one of the equal steps the sight
 * line is walked in, counted from the eye.
 */
constexpr int64_t GODOT_GROUND_TILE_CELLS = 8;   // cells along one edge of a tile: a metre on the lattice of 0.125 m, so a line falls little across it and a bank fills few tiles
constexpr int64_t GODOT_GROUND_NO_PLACE = 0;     // no place of the line has ground over it; the places between eye and point count from 1

}  // namespace ase::adp::godot
