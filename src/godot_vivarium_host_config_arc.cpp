/**
 * ASE GODOT ADAPTER - VIVARIUM HOST CONFIGURATION OF THE SKY ARC PORTS
 *
 * @file        godot_vivarium_host_config_arc.cpp
 * @brief       read_arc_ports - the names of the three ports of the sky arc, see godot_vivarium_host_config_arc.hpp
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6.
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

#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/godot_vivarium_host_config_arc.hpp>

#include <godot_cpp/variant/string.hpp>

#include <initializer_list>
#include <string>
#include <utility>

namespace ase::adp::godot {

bool read_arc_ports(const ::godot::Dictionary& ports, VivariumStartConfig& start, const char*& failed) {
    // flora.arc.v1, flora.almanac.v1 and flora.event_code.v1 (N0448): the key of each in section ports and the name
    // the start configuration gives its host port.
    struct Port {
        const char* key;
        const char* missing;
        std::string* name;
    };
    for (const Port& port : std::initializer_list<Port>{{"arc", "ports.arc", &start.arc_port},
                                                       {"almanac", "ports.almanac", &start.almanac_port},
                                                       {"event_code", "ports.event_code", &start.event_code_port}}) {
        ::godot::String text;
        if (!read_text(ports, port.key, text)) {
            failed = port.missing;
            return false;
        }
        *port.name = utf8_of(text);
    }
    return true;
}

}  // namespace ase::adp::godot
