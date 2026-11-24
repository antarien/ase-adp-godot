#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the cap port key, the cap role and the cap node the adapter reads and draws
 * @description The cap of a fruit body (DSGN_018 ORGXFORM, usecase:101362 scale the organ templates): ase-plant
 *              carries the cap radius of its species (PLT_SPEC_CAP_RADIUS), ase-phytomer passes it on for the
 *              individual of a patch (PHY_CAP_RADIUS), ase-pl-flora files one record per patch whose fruit body
 *              has a cap on its port flora.cap.v1, its object id the patch and its one value cap_radius in metres
 *              (FLORA_HOST_PORT_CAP, FLORA_HOST_KEY_CAP_RADIUS of ase-pl-flora types/cap/types.hpp, repeated here
 *              because the adapter reads the port and never links the plugin). GodotHostResourceManager merges
 *              the value into VivariumPatchView::cap_radius; the plant scene draws the shape of the cap role, while
 *              its file is missing the unit fruit body of the template port (GODOT_TMPL_FNGS), at the stem base of
 *              the patch, scaled so its widest reach equals the radius the record carried.
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter port/types.hpp and scene/types.hpp are included by more
 *              than a hundred own units of the avt build and take no new line (header ratchet, operator decision
 *              2026-11-21). These names are read by the cap reader of GodotHostResourceManager, by the plant look
 *              and scene that bind and draw the cap and by their tests: the cap rebuilds those units only.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_018
 * @usecase     usecase:101362 - scale the organ templates
 * @actor       Organ Transform Shaper (ORGATRASHA)
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

#include <cstdint>

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: cap = the cap of a fruit body, the dome over its hymenium; its radius is the half width of the cap
 * at its margin.
 */
constexpr const char* GODOT_KEY_CAP_RADIUS = "cap_radius";     // cap record: the cap radius of the fruit body of the patch, metres (PHY_CAP_RADIUS)
constexpr const char* GODOT_ROLE_CAP = "fungus.";              // template role of the fruit body a cap patch shows: the prefix before the catalog id of its species (fungus.amanita_phalloides), stand-in the unit fruit body GODOT_TMPL_FNGS
constexpr const char* GODOT_PLANT_CAPS = "PlantCaps";          // the fruit bodies of all cap patches beside the seeds: one child per species the stage names a cap role for, each body at the stem base of its patch
constexpr const char* GODOT_PLANT_CAP_KIND = "Cap_";           // name of the child of GODOT_PLANT_CAPS for one species, before its catalog id (Cap_amanita_phalloides): its MultiMesh holds the template of that species' cap role
constexpr uint32_t GODOT_COLOR_STAND_IN_CAP = 0x8F9A5CFFu;      // stand-in of GODOT_ROLE_CAP: the olive green cap of the death cap

}  // namespace ase::adp::godot
