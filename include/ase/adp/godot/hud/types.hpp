#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth collector of the HUD purpose headers of the care view
 * @description A pure collector of the thirteen purpose headers the HUD constants were dedicated into: debug,
 *              drag, gap, hand, log, motion, ring, sign, sprite, style, touch, weather and window. It holds no
 *              constant of its own; every value stands in exactly one purpose header, and a reader includes the
 *              narrowest one it needs.
 *
 *              WHY A COLLECTOR: this header was included by more than a hundred own units of the avt build and
 *              takes no new line (header ratchet, operator decision 2026-11-21); it was dedicated by purpose into
 *              sub-headers and ends as a pure collector of them. A new purpose gets a sub-header of its own,
 *              never a line here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-22
 * @modified    2026-11-25
 * @version     00.00.22.00022 [core]
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

#include "ase/adp/godot/hud/debug/types.hpp"
#include "ase/adp/godot/hud/drag/types.hpp"
#include "ase/adp/godot/hud/gap/types.hpp"
#include "ase/adp/godot/hud/hand/types.hpp"
#include "ase/adp/godot/hud/log/types.hpp"
#include "ase/adp/godot/hud/motion/types.hpp"
#include "ase/adp/godot/hud/ring/types.hpp"
#include "ase/adp/godot/hud/sign/types.hpp"
#include "ase/adp/godot/hud/sprite/types.hpp"
#include "ase/adp/godot/hud/style/types.hpp"
#include "ase/adp/godot/hud/touch/types.hpp"
#include "ase/adp/godot/hud/weather/types.hpp"
#include "ase/adp/godot/hud/window/types.hpp"
