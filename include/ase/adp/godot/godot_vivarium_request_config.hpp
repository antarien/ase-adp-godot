#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM REQUEST CONFIGURATION
 *
 * @file        godot_vivarium_request_config.hpp
 * @brief       The reader of the request ports and of every submit operation of the start configuration
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3: the view writes to the host through request ports, each a
 *              port of its own for one request class (a level, a spade stroke, a hoe stroke, the pace, a seek, a
 *              date jump, an event jump, the relief, the place of the camera), and names the operation of each
 *              submit by the word the start file gives. read_request_config (godot_vivarium_request_config.cpp)
 *              fills them once at the start; load_host_config (godot_vivarium_host_config.cpp) reads the record
 *              ports beside it. A header of its own, so only these two units see it (WRFL_ASE_TIER_WIPE).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.01.00001 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_start_config.hpp>

#include <godot_cpp/variant/dictionary.hpp>

namespace ase::adp::godot {

/**
 * Reads the request ports of the section ports and every operation of the section operations from the start file
 * into `start`. Returns nullptr when everything stands, else the key that is missing or malformed, for fail_config.
 */
const char* read_request_config(const ::godot::Dictionary& root, VivariumStartConfig& start);

}  // namespace ase::adp::godot
