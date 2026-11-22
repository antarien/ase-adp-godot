#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the ground and stage codes of a patch and the debug jumps of the care view
 * @description All compile-time constants of the patch sub-type; NO runtime values - those belong in the views.
 *              The ground codes and words of a patch (ase-pl-flora FLORA_SOIL_*), the stage count the view has a
 *              word and a colour for, the empty targets and date parts of a debug jump, the default pace and the
 *              code of a stage outside the documented ones.
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter types.hpp is included by more than a hundred own units of
 *              the avt build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated
 *              by purpose into sub-headers and stays a pure collector of them. These values are read by the patch
 *              reader, the patch views and the debug layer.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-22
 * @modified    2026-11-22
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

/**
 * ABBREVIATIONS: none.
 */

/**
 * The ground of a patch: the code the lay request carries and the patch record returns
 * (ase-pl-flora FLORA_SOIL_*), and the word the start configuration names it by.
 */
constexpr uint8_t GODOT_SOIL_MEADOW = 0;                      // meadow soil
constexpr uint8_t GODOT_SOIL_FOREST = 1;                      // forest floor
constexpr uint8_t GODOT_SOIL_COUNT = 2;                       // grounds the view has a material for
constexpr const char* GODOT_SOIL_WORD_MEADOW = "meadow";      // word of GODOT_SOIL_MEADOW in the start configuration
constexpr const char* GODOT_SOIL_WORD_FOREST = "forest";      // word of GODOT_SOIL_FOREST in the start configuration
constexpr const char* GODOT_KEY_AGE_DAYS = "age_days";        // patch of the start configuration, self-test lay and patch lay request: the Startstand, game days since germination at the world start, 0 for a seed (FLORA_HOST_KEY_AGE_DAYS; usecase:100390); ase-pl-flora judges the bounds

/**
 * The stage of a patch (usecase:101635 - show the plant stage, D2455). The patch record carries the stage
 * code of ase-plant unchanged; the stage record on flora.stage.v1 carries the four codes themselves
 * (GODOT_KEY_STGE_*). read_snapshot turns the stage of every patch into its position among those four keys,
 * in the order the keys are read - seed, sprout, grow, mature - and sets the patch's stage marks by
 * equality with them, as for the condition. The view holds no stage code of its own; a stage that equals
 * none of the four codes is shown as unknown, never mapped onto a known one.
 */
constexpr uint8_t GODOT_STAGE_COUNT = 4;      // stages the view has a word and a colour for: the codes the stage record carries
constexpr uint8_t GODOT_STAGE_NONE = UINT8_MAX;  // debug jump: no stage to jump to, the jump names an hour
constexpr double GODOT_HOUR_NONE = -1.0;      // debug jump: no hour of the game day to jump to, the jump names a stage
constexpr double GODOT_YEARS_NONE = 0.0;      // debug date jump: no VECTUM to jump ahead, the jump names a stage or an hour
constexpr uint8_t GODOT_ECLIPSE_NONE = 0u;    // debug eclipse jump: no kind to land on - ase-ephemeris ECLIPSE_NONE, the jump names another target
constexpr uint8_t GODOT_ECLIPSE_KIND_MAX = 4u;  // highest kind code of ase-ephemeris (ECLIPSE_LUNAR_TOTAL, types.hpp of ase-ephemeris)
constexpr uint8_t GODOT_ECLIPSE_SOLAR_MAX = 2u; // highest solar kind code of ase-ephemeris (ECLIPSE_SOLAR_TOTAL); kinds above it are lunar
constexpr uint8_t GODOT_DATE_PART_YEAR = 0u;    // debug date entry: the VECTUM
constexpr uint8_t GODOT_DATE_PART_MONTH = 1u;   // debug date entry: the LUNBREX of the VECTUM
constexpr uint8_t GODOT_DATE_PART_DAY = 2u;     // debug date entry: the TUMBRAE of the LUNBREX
constexpr uint8_t GODOT_DATE_PART_HOUR = 3u;    // debug date entry: the whole HORAX of the TUMBRAE
constexpr uint8_t GODOT_DATE_PARTS = 4u;        // parts of a SURA date the entry holds
constexpr uint8_t GODOT_DATE_PART_NONE = UINT8_MAX;  // debug date entry: a field or step that names no part
constexpr const char* GODOT_DATE_KEY_YEAR = "year";        // debug.date part word of the VECTUM
constexpr const char* GODOT_DATE_KEY_MONTH = "month";      // debug.date part word of the LUNBREX
constexpr const char* GODOT_DATE_KEY_DAY = "month_day";    // debug.date part word of the TUMBRAE
constexpr const char* GODOT_DATE_KEY_HOUR = "hour";        // debug.date part word of the HORAX
constexpr double GODOT_DATE_YEAR_FIRST = 0.0;   // first VECTUM of a run as the clock record counts it (ase-calendar CAL_DATE_SEEK_YEAR_FIRST)
constexpr double GODOT_DATE_FIRST = 1.0;        // first LUNBREX and first TUMBRAE as the clock record counts them (CAL_DATE_SEEK_DATE_FIRST)
constexpr double GODOT_DATE_HOUR_FIRST = 0.0;   // first HORAX of a TUMBRAE (CAL_DATE_SEEK_HOUR_FIRST)
constexpr double GODOT_PACE_NONE = 0.0;       // pace request: the host's clock back on its default pace
constexpr uint8_t GODOT_CODE_UNKNOWN = 255u;  // a stage outside the documented codes

}  // namespace ase::adp::godot
