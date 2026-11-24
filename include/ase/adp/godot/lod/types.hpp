#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the record keys of the level port the adapter reads
 * @description The graded level of a plant body (PLAN_ASE_VIVARIUM_PHASE_09_TREEGEN T.10, Detailstufen;
 *              usecase:101657 grade plants by picture height): ase-render-data grades every plant body by the height
 *              of its picture (RND_LOD_LEVEL, RND_LOD_FADE under the body, the codes RND_CONST_LOD_LEVEL_* on GLOBAL),
 *              ase-pl-flora files one record per graded patch on its port flora.lod.v1, its object id the patch and
 *              its five values level, fade, view, tile and cnpy (FLORA_HOST_PORT_LOD and the keys of ase-pl-flora
 *              types/lod/types.hpp, repeated here because the adapter reads the port and never links the plugin).
 *              GodotHostResourceManager merges them into VivariumPatchView; the plant scene hands the body over to
 *              its card by them (level_share, godot_plant_impostor.hpp), never by a distance rule of its own.
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter port/types.hpp and scene/types.hpp are included by more
 *              than a hundred own units of the avt build and take no new line (header ratchet, operator decision
 *              2026-11-21). These names are read by the level reader of GodotHostResourceManager and its test only.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @usecase     usecase:101657 - grade plants by picture height
 * @actor       Picture Height Grader (PICTHGTGRA)
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

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: lod = level of detail, the graded level of a plant body; view = the level of the view card,
 * tile = the level of the billboard, cnpy = the level of the canopy, each as the code ase-render-data grades with.
 */
constexpr const char* GODOT_KEY_LOD_LEVEL = "lod_level";  // level record: the graded level of the plant body of the patch, one of the RND_CONST_LOD_LEVEL_* codes
constexpr const char* GODOT_KEY_LOD_FADE = "lod_fade";    // level record: the share handed over to the next coarser level, RND_CONST_LOD_FADE_NONE..RND_CONST_LOD_FADE_FULL
constexpr const char* GODOT_KEY_LOD_VIEW = "lod_view";    // level record: RND_CONST_LOD_LEVEL_VIEW, the level from which the view card stands in for the body
constexpr const char* GODOT_KEY_LOD_TILE = "lod_tile";    // level record: RND_CONST_LOD_LEVEL_TILE, the level of the billboard
constexpr const char* GODOT_KEY_LOD_CNPY = "lod_cnpy";    // level record: RND_CONST_LOD_LEVEL_CNPY, the level of the canopy
constexpr double GODOT_LOD_CODE_STEP = 1.0;               // distance of two neighbouring codes of RND_CONST_LOD_LEVEL_*: FULL 0 to CNPY 5 follow each other by one (hub_constants.json), so the level whose fade leads into the view level lies one step below it

}  // namespace ase::adp::godot
