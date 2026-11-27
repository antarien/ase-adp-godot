#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM HOST CONFIGURATION OF THE SKY ARC PORTS
 *
 * @file        godot_vivarium_host_config_arc.hpp
 * @brief       read_arc_ports - the names of the three ports of the sky arc from the start configuration
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6. The start configuration (vivarium_start.json, section
 *              ports) names the host port of every record the view reads; the sky arc has three: `arc`
 *              (flora.arc.v1, one record every frame), `almanac` (flora.almanac.v1, one record per occurrence) and
 *              `event_code` (flora.event_code.v1, the codes of the calendar's kinds and rings on two records). A missing
 *              name refuses the configuration with the key named, as every port of the section does. Cut from
 *              godot_vivarium_host_config.cpp by its size band.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_host_start_config.hpp>

#include <godot_cpp/variant/dictionary.hpp>

namespace ase::adp::godot {

/** Reads the three arc port names of `ports` into `start`; false with the missing key ("ports.<key>") in `failed`. */
bool read_arc_ports(const ::godot::Dictionary& ports, VivariumStartConfig& start, const char*& failed);

}  // namespace ase::adp::godot
