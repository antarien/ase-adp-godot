/**
 * ASE GODOT ADAPTER - VIVARIUM REQUEST CONFIGURATION
 *
 * @file        godot_vivarium_request_config.cpp
 * @brief       read_request_config - the request ports the view writes to and every submit operation
 * @description PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3 and PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8: every request the
 *              view submits to the host goes through a port of its own, one request class per port, and names its
 *              operation by the word of the start file. This unit reads both once at the start; a missing or broken
 *              key is returned by its name, and the view fails the configuration with it (fail_config in
 *              load_host_config). The record ports the view reads stay in godot_vivarium_host_config.cpp.
 *
 *              It reads into the start configuration only, so it is no method of the view: its reason to change
 *              is a new request class, while the record ports change with what an owner offers.
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

#include <ase/adp/godot/godot_vivarium_request_config.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_support.hpp>

#include <godot_cpp/variant/string.hpp>

#include <string>

namespace ase::adp::godot {

namespace {

/** One name of a section into `name`; false when it is missing or no text. */
bool read_name(const ::godot::Dictionary& section, const char* key, std::string& name) {
    ::godot::String text;
    if (!read_text(section, key, text)) {
        return false;
    }
    name = utf8_of(text);
    return true;
}

}  // namespace

const char* read_request_config(const ::godot::Dictionary& root, VivariumStartConfig& start) {
    ::godot::Dictionary ports;
    if (!read_section(root, "ports", ports)) {
        return "ports";
    }
    // The level requests of the player (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8 point 2): a port of their
    // own, the patch port takes only pours.
    if (!read_name(ports, "level", start.level_port)) {
        return "ports.level";
    }
    // The flatten requests of the player at a place (03.8): a port of their own, they name no patch.
    if (!read_name(ports, "flatten", start.flatten_port)) {
        return "ports.flatten";
    }
    // The spade strokes of the player at a place: one port per direction, so the direction is never a branch.
    if (!read_name(ports, "lower", start.lower_port)) {
        return "ports.lower";
    }
    if (!read_name(ports, "raise", start.raise_port)) {
        return "ports.raise";
    }
    // The hoe strokes at a place (soltar, furar; B0649): a port of their own, they loosen the topsoil and move
    // no height.
    if (!read_name(ports, "loosen", start.loosen_port)) {
        return "ports.loosen";
    }
    // The pace requests of the debug mode (the time lapse of this host's clock): a port of their own,
    // they name no object.
    if (!read_name(ports, "pace", start.pace_port)) {
        return "ports.pace";
    }
    // The seek requests of the debug mode (this host's clock moved to a computed moment): a port of
    // their own beside the time lapse, they name no object.
    if (!read_name(ports, "seek", start.seek_port)) {
        return "ports.seek";
    }
    // The real position of the host (its real seconds since the start, sleep included), filed every
    // frame: a port of its own, it names no object; a return from the background is one such filing.
    if (!read_name(ports, "real", start.real_port)) {
        return "ports.real";
    }
    // The date requests of the debug mode (this host's clock moved to a SURA date): a port of their own
    // beside the seek, they name no object; ase-calendar turns the date into the hour.
    if (!read_name(ports, "date", start.date_port)) {
        return "ports.date";
    }
    // The event requests of the debug mode (this host's clock moved to the next occurrence of a kind): a port
    // of their own, one request class per port, they name no object; ase-calendar finds the occurrence.
    if (!read_name(ports, "event", start.event_port)) {
        return "ports.event";
    }
    // The relief of the ground (the height amplitude and the ground level of the whole terrain): a port
    // of its own, it names no object.
    if (!read_name(ports, "elevation", start.elevation_port)) {
        return "ports.elevation";
    }
    // The place of the view's camera (03.11 point 7, the level of detail): a port of its own, it names no
    // object.
    if (!read_name(ports, "view", start.view_port)) {
        return "ports.view";
    }

    ::godot::Dictionary operations;
    if (!read_section(root, "operations", operations)) {
        return "operations";
    }
    if (!read_name(operations, "irrigate", start.op_irrigate)) {
        return "operations.irrigate";
    }
    if (!read_name(operations, "prune", start.op_prune)) {
        return "operations.prune";
    }
    if (!read_name(operations, "level", start.op_level)) {
        return "operations.level";
    }
    if (!read_name(operations, "flatten", start.op_flatten)) {
        return "operations.flatten";
    }
    if (!read_name(operations, "lower", start.op_lower)) {
        return "operations.lower";
    }
    if (!read_name(operations, "raise", start.op_raise)) {
        return "operations.raise";
    }
    if (!read_name(operations, "loosen", start.op_loosen)) {
        return "operations.loosen";
    }
    if (!read_name(operations, "pace", start.op_pace)) {
        return "operations.pace";
    }
    if (!read_name(operations, "seek", start.op_seek)) {
        return "operations.seek";
    }
    if (!read_name(operations, "seek_date", start.op_seek_date)) {
        return "operations.seek_date";
    }
    // The event jump of the debug mode: the next occurrence of a kind, ase-calendar computes its date and lead;
    // the view names the kind only.
    if (!read_name(operations, "seek_event", start.op_seek_event)) {
        return "operations.seek_event";
    }
    if (!read_name(operations, "real", start.op_real)) {
        return "operations.real";
    }
    if (!read_name(operations, "shape", start.op_shape)) {
        return "operations.shape";
    }
    if (!read_name(operations, "view", start.op_view)) {
        return "operations.view";
    }
    if (!read_name(operations, "lay", start.op_lay)) {
        return "operations.lay";
    }
    return nullptr;
}

}  // namespace ase::adp::godot
