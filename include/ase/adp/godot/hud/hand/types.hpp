#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the hand slot: its modes, the terrain verbs and the acts of the hand
 * @description The hand slot (06.4 HandSlot, the mockup's hand()): the modes whose lines it speaks, named as
 *              hud.modes, the ops of a terrain verb (the token hud_verb_<v>_op), the hold word of a tool at work
 *              (hud_item_<id>_hold) and the acts of the hand, the stow button and a verb chip.
 *
 *              WHY A HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated by
 *              purpose and stays a pure collector. The hand is read by the hand slot and its bridge.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
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

constexpr const char* GODOT_HUD_MODE_REST = "rest";                   // nothing chosen: the empty hand may say where to start
constexpr const char* GODOT_HUD_MODE_WATER = "water";                 // the can: pour, its arcs, the soil before and after
constexpr const char* GODOT_HUD_MODE_PRUNE = "prune";                 // the pruning knife: the shoots left to cut
constexpr const char* GODOT_HUD_MODE_SOW = "sow";                     // the planter: the seeds left in the pouch
constexpr const char* GODOT_HUD_MODE_TERRAIN = "terrain";             // a ground tool: its verbs above the hand
constexpr const char* GODOT_HUD_MODE_SAP = "sap";                     // the auger: the spouts left for a tap
constexpr const char* GODOT_HUD_MODE_PLUG = "plug";                   // the peg: the sap the vessel holds
constexpr const char* GODOT_HUD_VERB_OP_BED = "bed";                  // lays a brook bed between two points
constexpr const char* GODOT_HUD_VERB_OP_LEVEL = "level";              // levels the whole area
constexpr const char* GODOT_HUD_VERB_OP_MEASURE = "measure";          // measures one point
constexpr const char* GODOT_HUD_VERB_TOKEN = "hud_verb_";            // theme: the head of every verb token hud_verb_<v>_*
constexpr const char* GODOT_HUD_VERB_OP_TAIL = "_op";                 // theme: hud_verb_<v>_op, the terrain op of the verb
constexpr const char* GODOT_HUD_VERB_TONE_TAIL = "_tone";             // theme: hud_verb_<v>_tone, the domain its hexagon draws in
constexpr const char* GODOT_HUD_VERB_BRUSH_TAIL = "_brush_m";         // theme: hud_verb_<v>_brush_m, the radius a brush verb works under, metres
constexpr int64_t GODOT_HUD_GROUND_NO_SAMPLE = -1;                    // UiGround: a lattice point the terrain port carried no sample for
constexpr const char* GODOT_HUD_ITEM_HOLD_NONE = "none";              // a tool at work: the hand offers its use
constexpr const char* GODOT_HUD_ACT_POUR = "pour";                    // the can in the hand in the mode water: pours while held
constexpr const char* GODOT_HUD_ACT_STOW = "drop";                    // the stow button: the thing goes back where it came from
constexpr const char* GODOT_HUD_ACT_VERB = "terr";                    // a verb chip; its verb goes with it

}  // namespace ase::adp::godot
