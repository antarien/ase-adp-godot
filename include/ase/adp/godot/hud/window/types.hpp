#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the window of the vivarium: its display, its feature and its log line
 * @description The Godot feature of a handheld OS, the name of the display server that opens no window, and the
 *              text room and decimals of the measured values the window line names.
 *
 *              WHY A HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated by
 *              purpose and stays a pure collector. The window is read by the start of the client window only.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_016
 * @usecase     usecase:101019 - start the client window
 * @actor       Vivarium Steward (VIVARIUSTE)
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

constexpr const char* GODOT_FEATURE_MOBILE = "mobile";      // Godot feature of a handheld OS: the window is the screen, its safe area applies
constexpr const char* GODOT_DISPLAY_HEADLESS = "headless";  // name of the display server that opens no window (usecase:101019 start the client window)
constexpr uint32_t GODOT_WINDOW_NUMBER_CHARS = 32u;         // text room of one measured value in the window line
constexpr uint32_t GODOT_WINDOW_DECIMALS = 2u;              // decimals of a surface size or screen scale in the window line

}  // namespace ase::adp::godot
