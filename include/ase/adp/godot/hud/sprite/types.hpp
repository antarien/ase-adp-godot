#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the path reader of a carried thing's drawing
 * @description The drawing of a carried thing (06.3 UiItemSprite): item_art.json writes each path as SVG path
 *              data; the reader knows the commands generate-item-art.cjs lets through and flattens every curve.
 *
 *              WHY A HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated by
 *              purpose and stays a pure collector. The path reader is read by UiItemSprite only.
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

constexpr const char* GODOT_ITEM_PATH_COMMANDS = "MLHVCQAZmlhvcqaz";  // the path commands of item_art.json (generate-item-art.cjs PATH_COMMANDS)
constexpr const char* GODOT_ITEM_PATH_SEPARATORS = " ,\t\r\n";       // characters between two numbers of SVG path data
constexpr int32_t GODOT_ITEM_CURVE_POINTS = 16;                       // points one Bezier piece or one arc is flattened to
constexpr int32_t GODOT_ITEM_POLYGON_MIN = 3;                         // fewest corners a filled outline needs
constexpr double GODOT_ITEM_POINT_EPSILON = 1.0e-4;                   // view-box distance at which two outline points count as one

}  // namespace ase::adp::godot
