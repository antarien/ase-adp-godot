#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM START MODE
 *
 * @file        godot_vivarium_start_mode.hpp
 * @brief       The four values of VivariumView::start_mode_: no choice yet, online, offline new, offline resumed
 * @description The start screen holds the world until the keeper chose how it starts (operator order of
 *              2026-11-19): online or offline, and offline a new world or the last kept state. The choice lies
 *              in VivariumView::start_mode_ as one of these values; boot_host hands it to the host through
 *              VivariumStartConfig::start_fresh, which is true for GODOT_START_MODE_OFFLINE_INITIAL alone, and the
 *              last-state loader of the kernel then reads no file of the last state.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-19
 * @modified    2026-11-19
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <cstdint>

namespace ase::adp::godot {

constexpr uint8_t GODOT_START_MODE_NONE = 0;             // the start screen waits for the choice; no host runs
constexpr uint8_t GODOT_START_MODE_ONLINE = 1;           // the world starts online
constexpr uint8_t GODOT_START_MODE_OFFLINE_INITIAL = 2;  // offline, a new world: no file of the last state is read
constexpr uint8_t GODOT_START_MODE_OFFLINE_RESUME = 3;   // offline, the last kept state is loaded

}  // namespace ase::adp::godot
