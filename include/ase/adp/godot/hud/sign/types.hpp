#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the anchors a sign stands at and the bar value of a sign without a bar
 * @description The anchors a sign stands at (06.4 SignMarker, the token hud_sign_<id>_at) and the bar value of a
 *              sign without a bar (hud_sign_<id>_bar).
 *
 *              WHY A HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated by
 *              purpose and stays a pure collector. The anchors are read by the sign marker and its rules.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @usecase     usecase:100370 - wilt from drought
 * @actor       Hänge-Birke (Betula pendula)
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

namespace ase::adp::godot {

constexpr const char* GODOT_HUD_SIGN_AT_CROWN = "crown";              // the mockup's crownPt(x, y) over the trunk
constexpr const char* GODOT_HUD_SIGN_AT_TRUNK = "trunk";              // an offset from the foot of the trunk
constexpr const char* GODOT_HUD_SIGN_AT_BROOK = "brook";              // the share x along the brook, y above it
constexpr const char* GODOT_HUD_SIGN_AT_SCREEN = "screen";            // a point of the mockup's screen
constexpr const char* GODOT_HUD_SIGN_BAR_NONE = "none";               // the sign has no bar

}  // namespace ase::adp::godot
