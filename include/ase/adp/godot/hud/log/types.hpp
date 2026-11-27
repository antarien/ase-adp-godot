#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the log line of the vivarium: decimals and the schedules it counts
 * @description The decimals of a measured value in a log line, the schedule whose runs a log line counts as
 *              steps (the modules' 1 Hz step), and the game-time schedule whose runs close a measured cycle.
 *
 *              WHY A HEADER OF ITS OWN: hud/types.hpp is included by more than a hundred own units of the avt
 *              build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated by
 *              purpose and stays a pure collector. The log form is read by the units that write a log line.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_046
 * @usecase     usecase:101120 - clock the growth cycle
 * @actor       Growth Clock Controller (GROWCLOCON)
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

constexpr int64_t GODOT_LOG_DECIMALS = 3;                   // decimals of a measured value in a log line
constexpr const char* GODOT_LOG_STEP_SCHEDULE = "Dissemination";  // schedule whose runs a log line counts as steps= (the modules' 1 Hz step)
constexpr const char* GODOT_DAY_TURN_SCHEDULE = "GameDawn";        // game-time schedule ase-calendar requests once per crossed TUMBRAE; its runs close a measured cycle

}  // namespace ase::adp::godot
