#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the forecast key of an unreachable growing stage the adapter reads and shows
 * @description A shoot whose growth time ends before its stem reaches the seedling bound never reaches CRESCENDO:
 *              ase-phytomer files the growing days it lacks (PHY_SIZE_SHRT_D), ase-plant publishes them for the
 *              stand (PLT_GROW_SHRT_D) and ase-pl-flora files them on the forecast record of the patch
 *              (FLRA_HOST_KEY_GROW_SHRT_D of ase-pl-flora types/forecast/types.hpp, repeated here because the
 *              adapter reads the port and never links the plugin). GodotHostResourceManager keeps the value per
 *              patch (stage_short_d), and the HUD bridge shows the stage unreachable instead of filling the gap
 *              with the days of the mockup (register D2676).
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter port/types.hpp is included by more than a hundred own
 *              units of the avt build and takes no new line (header ratchet, operator decision 2026-11-21). This
 *              key is read by the forecast reader of GodotHostResourceManager only: a change rebuilds that unit.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-27
 * @modified    2026-11-27
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

constexpr const char* GODOT_KEY_GROW_SHORT_D = "grow_short_d";  // forecast record: growing days the stand lacks to CRESCENDO when its growth time ends first (PLT_GROW_SHRT_D), filed only while CRESCENDO is unreachable
constexpr double GODOT_SHORT_NONE_D = 0.0;                       // growing days of lack of a reachable stage: a value at or below it is no lack

}  // namespace ase::adp::godot
