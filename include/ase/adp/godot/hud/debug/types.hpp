#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the tabs of the debug layer and the index no chip has
 * @description The debug layer (06.4 DebugLayer, the mockup's debugLayer()): its four tabs in the order of
 *              texts.hud.debug_tabs, and the index of a hit on no chip of a kind.
 *
 *              WHY A HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated by
 *              purpose and stays a pure collector. The tabs are read by the debug layer and its input.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_046
 * @usecase     usecase:101120 - clock the growth cycle
 * @actor       Growth Clock Controller (GROWCLOCON)
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

constexpr uint32_t GODOT_HUD_DEBUG_TAB_TEMPO = 0u;                    // the time lapse: debug.buttons, flora.pace.v1
constexpr uint32_t GODOT_HUD_DEBUG_TAB_SALTO = 1u;                    // the jumps: debug.jumps, flora.seek.v1 and flora.date.v1
constexpr uint32_t GODOT_HUD_DEBUG_TAB_DATA = 2u;                     // the free date entry: debug.date, flora.date.v1
constexpr uint32_t GODOT_HUD_DEBUG_TAB_RELEVO = 3u;                   // the relief: debug.reliefs, flora.elevation.v1
constexpr uint32_t GODOT_HUD_DEBUG_TABS = 4u;                         // tabs of the drawer
constexpr uint32_t GODOT_HUD_DEBUG_NONE = UINT32_MAX;                 // a hit on no chip of that kind

}  // namespace ase::adp::godot
