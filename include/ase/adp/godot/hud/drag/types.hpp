#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for drag and drop: places, place words, the toast and what a release did
 * @description Drag and drop (06.7 DragLayer, the mockup's dropTarget and dropItem): the places a thing can go
 *              besides a container (tg.c), the place words of texts.hud (tg.lab), the words and colours of the
 *              toast after a release, and what a release did.
 *
 *              WHY A HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated by
 *              purpose and stays a pure collector. Drag and drop is read by the drag layer and the carry bridge.
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

constexpr const char* GODOT_HUD_PLACE_HAND = "hand";                  // the hand slot: the thing is equipped
constexpr const char* GODOT_HUD_PLACE_GROUND = "ground";              // the ground of the world, the tool place included
constexpr const char* GODOT_HUD_WHERE_HAND = "where.hand";            // texts.hud: in the hand
constexpr const char* GODOT_HUD_WHERE_BELT = "where.belt";            // texts.hud: on the belt, either holster
constexpr const char* GODOT_HUD_WHERE_PACK = "where.ranzen";          // texts.hud: in the leather pack
constexpr const char* GODOT_HUD_WHERE_POUCH = "where.bolsa";          // texts.hud: in the linen pouch
constexpr const char* GODOT_HUD_WHERE_RACK = "where.rack";            // texts.hud: at the tool place
constexpr const char* GODOT_HUD_WHERE_GROUND = "where.ground";        // texts.hud: on the ground
constexpr const char* GODOT_HUD_WHERE_TRUNK = "where.trunk";          // texts.hud: on the trunk, the vessel under the tap
constexpr const char* GODOT_HUD_WORD_DROP_AT = "drop_at";             // texts.hud: {item} · {where} after an accepted drop
constexpr const char* GODOT_HUD_WORD_FIT = "fit_";                    // texts.hud: the head of a refusal word fit_<c>
constexpr const char* GODOT_HUD_WORD_ITEM_HEAD = "items.";            // texts.hud: the head of an item's word items.<id>.
constexpr const char* GODOT_HUD_WORD_ITEM_TAIL = ".name";             // texts.hud: the tail of an item's word .<field>
constexpr const char* GODOT_HUD_WORD_ITEM_CASE = ".case";             // texts.hud: the tail of an item's case items.<id>.case
constexpr const char* GODOT_HUD_COLOUR_DROP = "DOMAIN_GROUND";        // the toast of an accepted drop (toast(..., 'ground'))
constexpr const char* GODOT_HUD_COLOUR_REFUSED = "DOMAIN_ALERT";      // the toast of a refused drop (toast(tg.bad, 'alert'))
constexpr const char* GODOT_HUD_DRAG_TAP = "tap";                     // the finger never moved past the slop: a tap on the thing
constexpr const char* GODOT_HUD_DRAG_SEND = "send";                   // the module allowed the place: the move goes to flora.stow.v1
constexpr const char* GODOT_HUD_DRAG_REFUSED = "refused";             // the module refused the place: its word was toasted
constexpr const char* GODOT_HUD_DRAG_HELD = "held";                   // no place under the finger or no verdict: the thing stays

}  // namespace ase::adp::godot
