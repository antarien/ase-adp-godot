/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW CAMP CONFIGURATION
 *
 * @file        godot_vivarium_camp_config.cpp
 * @brief       read_camp_config - the ports of ase-pl-ingle, the camp place and every part of the fire
 * @description PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.0, T.3 and T.7: the start configuration names the five ports of
 *              ase-pl-ingle the view talks to (camp place, hull, part, body, refuel), the place of the camp measured
 *              on the height cells of ase-terrain (entry camp, T.0) and one entry per body of the fire (camp.parts).
 *              Every key is required: a missing or malformed one is returned by its name, and the view fails the
 *              configuration with it (fail_config in load_host_config). follow_camp (godot_vivarium_camp.cpp) lays
 *              them once the host stands.
 *
 *              It reads into the start configuration only, so it is no method of the view: its reason to change
 *              is the start file, read once at the start; follow_camp runs every frame.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_068
 * @category    ecs/module
 * @usecase     usecase:5436 - kindle fire
 * @actor       Funken (FNKN)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_camp.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

namespace {

constexpr double kCampIndexMax = 4294967295.0;  // the largest place or variant a uint32_t holds

/** True when `value` is a whole number from 0 to kCampIndexMax, the form of a place or a variant. */
bool whole_index(double value) {
    return value >= 0.0 && value <= kCampIndexMax && static_cast<double>(static_cast<uint32_t>(value)) == value;
}

/** One port name of the section ports into `port`; false when it is missing or no text. */
bool read_port(const ::godot::Dictionary& ports, const char* key, std::string& port) {
    ::godot::String text;
    if (!read_text(ports, key, text)) {
        return false;
    }
    port = utf8_of(text);
    return true;
}

/** One entry of camp.parts into `part`; nullptr when it stands whole, else the key that does not. */
const char* read_part(const ::godot::Variant& value, VivariumCampPartSpec& part) {
    if (value.get_type() != ::godot::Variant::DICTIONARY) {
        return "camp.parts[]";
    }
    const ::godot::Dictionary entry = value;
    ::godot::String text;
    if (!read_text(entry, "role", text) || text.is_empty()) {
        return "camp.parts[].role";
    }
    part.role = utf8_of(text);
    double variant = 0.0;
    if (!read_number(entry, "variant", variant) || !whole_index(variant)) {
        return "camp.parts[].variant";
    }
    part.variant = static_cast<uint32_t>(variant);
    if (!read_number(entry, "east", part.east) || !std::isfinite(part.east)) {
        return "camp.parts[].east";
    }
    if (!read_number(entry, "north", part.north) || !std::isfinite(part.north)) {
        return "camp.parts[].north";
    }
    if (!read_number(entry, "drop", part.drop) || !std::isfinite(part.drop)) {
        return "camp.parts[].drop";
    }
    // The turn the pyramid lays the part in (data/probes/n0541_camp_layout_probe.py); any rotation vector is a turn.
    if (!read_number(entry, "tx", part.tx) || !std::isfinite(part.tx)) {
        return "camp.parts[].tx";
    }
    if (!read_number(entry, "ty", part.ty) || !std::isfinite(part.ty)) {
        return "camp.parts[].ty";
    }
    if (!read_number(entry, "tz", part.tz) || !std::isfinite(part.tz)) {
        return "camp.parts[].tz";
    }
    if (!read_number(entry, "fall", part.fall) || !std::isfinite(part.fall) || !(part.fall > 0.0)) {
        return "camp.parts[].fall";
    }
    const ::godot::Variant fuel = entry.get("fuel", ::godot::Variant());
    if (fuel.get_type() != ::godot::Variant::BOOL) {
        return "camp.parts[].fuel";
    }
    part.fuel = static_cast<bool>(fuel);
    return nullptr;
}

}  // namespace

const char* read_camp_config(const ::godot::Dictionary& root, VivariumStartConfig& start) {
    // THE PORTS OF ase-pl-ingle (T.3, T.7): a unit set without the plugin boots, and the manager names the ports
    // it misses once the host stands; the keys themselves are the product's and stand in the start file.
    ::godot::Dictionary ports;
    if (!read_section(root, "ports", ports)) {
        return "ports";
    }
    if (!read_port(ports, "camp_place", start.camp_place_port)) {
        return "ports.camp_place";
    }
    if (!read_port(ports, "camp_hull", start.camp_hull_port)) {
        return "ports.camp_hull";
    }
    if (!read_port(ports, "camp_part", start.camp_part_port)) {
        return "ports.camp_part";
    }
    if (!read_port(ports, "camp_body", start.camp_body_port)) {
        return "ports.camp_body";
    }
    if (!read_port(ports, "camp_refuel", start.camp_refuel_port)) {
        return "ports.camp_refuel";
    }

    // THE CAMP PLACE (T.0): measured on the height cells of ase-terrain, never guessed; the patch of camp.place
    // is the frame every part is placed in. load_patches ran before, so the place is held against them.
    ::godot::Dictionary camp;
    if (!read_section(root, "camp", camp)) {
        return "camp";
    }
    if (!read_number(camp, "x", start.camp_x) || !std::isfinite(start.camp_x)) {
        return "camp.x";
    }
    if (!read_number(camp, "z", start.camp_z) || !std::isfinite(start.camp_z)) {
        return "camp.z";
    }
    double place = 0.0;
    if (!read_number(camp, "place", place) || !whole_index(place)) {
        return "camp.place";
    }
    start.camp_place = static_cast<uint32_t>(place);
    const uint32_t camp_place = start.camp_place;
    if (std::none_of(start.patches.begin(), start.patches.end(),
                     [camp_place](const VivariumPatchSpec& patch) { return patch.place == camp_place; })) {
        return "camp.place";
    }

    // EVERY BODY OF THE FIRE (T.3): one entry per stone, log, stick and the tinder. The role and variant name the
    // node of the export whose extras carry make, mass and hull; the place, drop and fall stand measured here.
    const ::godot::Variant parts_value = camp.get("parts", ::godot::Variant());
    if (parts_value.get_type() != ::godot::Variant::ARRAY) {
        return "camp.parts";
    }
    const ::godot::Array parts = parts_value;
    start.camp_parts.clear();
    for (int64_t i = 0; i < parts.size(); ++i) {
        VivariumCampPartSpec part;
        const char* missing = read_part(parts[i], part);
        if (missing != nullptr) {
            return missing;
        }
        start.camp_parts.push_back(part);
    }
    if (start.camp_parts.empty()) {
        return "camp.parts";
    }
    return nullptr;
}

}  // namespace ase::adp::godot
