#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the record key of the looseness port the adapter reads
 * @description The port terrain.loose.v1 (register N0456, CAUSA_ASE_TERRAIN Loosened soil): one record per macro
 *              cell of ase-terrain, filed by FloraHostCpctExptSystem of ase-pl-flora from the rows HUB_TERN_MCRO,
 *              with the cell's centre x and z (GODOT_KEY_X, GODOT_KEY_Z), its edge (GODOT_KEY_CELL_M) and the
 *              compaction share named here. GodotHostResourceManager::read_loose keeps the cells as the grid they
 *              form; the looseness the HUD's focus card and the crumbs draw is the complement of the share.
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter port/types.hpp is included by more than a hundred own
 *              units of the avt build and takes no new or changed line (header ratchet, operator decision
 *              2026-11-21); this key is read only by the looseness reader, so it stands in the narrowest header
 *              that reader includes.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-23
 * @modified    2026-11-23
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

namespace ase::adp::godot {

constexpr const char* GODOT_KEY_CPCT_SHARE = "cpct_share";  // loose record: compaction of one macro cell of ase-terrain, share 0 (freshly loosened) to 1 (root-limiting), as its owner keeps it (HUB_TERN_MCRO_CPCT, N0456); the looseness is its complement

}  // namespace ase::adp::godot
