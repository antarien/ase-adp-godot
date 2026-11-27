#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the parts of the care view a touch can land on, as the mockup's data-act
 * @description The grips, bodies and crosses of the pack and the pouch windows, a thing, the containers and the
 *              belt holsters, the sky band and its debug switch (06.7 BagWindow).
 *
 *              WHY A HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated by
 *              purpose and stays a pure collector. The acts are read by the hit test and the input of the HUD.
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

namespace ase::adp::godot {

constexpr const char* GODOT_HUD_ACT_PACK_GRIP = "win:ranzen";         // the leather pack's flap: drags the window
constexpr const char* GODOT_HUD_ACT_POUCH_GRIP = "win:bolsa";         // the linen pouch's neck: drags the window
constexpr const char* GODOT_HUD_ACT_WINDOW_BODY = "winbody";          // a window's body: takes the touch, does nothing
constexpr const char* GODOT_HUD_ACT_PACK_CLOSE = "bag";               // the pack's cross (and the bag icon): opens or closes it
constexpr const char* GODOT_HUD_ACT_POUCH_CLOSE = "pclose";           // the pouch's cross: closes it
constexpr const char* GODOT_HUD_ACT_ITEM = "item";                    // a thing; its key goes with it
constexpr const char* GODOT_HUD_CONTAINER_PACK = "ranzen";            // the leather pack as a place things lie in
constexpr const char* GODOT_HUD_CONTAINER_POUCH = "bolsa";            // the linen pouch as a place things lie in
constexpr const char* GODOT_HUD_CONTAINER_BELT_FIRST = "belt0";       // the left holster on the belt
constexpr const char* GODOT_HUD_CONTAINER_BELT_SECOND = "belt1";      // the right holster on the belt
constexpr const char* GODOT_HUD_ACT_NOOP = "noop";                    // an empty holster: takes the touch, does nothing
constexpr const char* GODOT_HUD_ACT_REST = "rest";                    // the scrim of the tool rack's list: back to the mode rest
constexpr const char* GODOT_HUD_ACT_SKY = "sky";                      // the sky band: opens the calendar sheet
constexpr const char* GODOT_HUD_ACT_DEBUG = "dbg";                    // the sky band's debug switch: shows or hides the debug layer
constexpr const char* GODOT_HUD_ITEM_ART_PACK = "ranzen";             // item_art.json: the drawing of the leather pack (BagIcon)

}  // namespace ase::adp::godot
