/**
 * ASE GODOT ADAPTER - HUD UI ACTIVITY ART
 *
 * @file        godot_hud_ui_activity_art.cpp
 * @brief       Reads every activity symbol of SHA's activity_art.json once and answers lookups by name
 * @description MIG_ASE_SKILL_RSM B7, see godot_hud_ui_activity_art.hpp. The file is read exactly like
 *              item_art.json is read by UiTheme::load: a missing or unreadable file or section refuses the
 *              configuration, there is no fallback.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_activity_art.hpp>
#include <ase/adp/godot/godot_config_read.hpp>

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace ase::adp::godot {

namespace {

/** The JSON object in the file at `path`; false when it does not parse to an object. */
bool read_json_file(const ::godot::String& path, ::godot::Dictionary& out) {
    const ::godot::Variant parsed = ::godot::JSON::parse_string(::godot::FileAccess::get_file_as_string(path));
    if (parsed.get_type() != ::godot::Variant::DICTIONARY) {
        return false;
    }
    out = parsed;
    return true;
}

}  // namespace

bool UiActivityArt::load(const ::godot::Dictionary& root, std::string& failed) {
    ::godot::Dictionary hud;
    ::godot::Dictionary distribution;
    ::godot::String path;
    ::godot::Dictionary file;
    if (!read_section(root, "hud", hud) || !read_section(hud, "distribution", distribution)) {
        failed = "hud.distribution";
        return false;
    }
    if (!read_text(distribution, "activities", path) || !read_json_file(path, file) ||
        !read_section(file, "activities", activities_)) {
        failed = "hud.distribution.activities";
        return false;
    }
    if (!read_section(file, "places", places_)) {
        failed = "hud.distribution.activities: places";
        return false;
    }
    return true;
}

bool UiActivityArt::art(const std::string& name, ::godot::Dictionary& out) const {
    return !name.empty() && read_section(activities_, name.c_str(), out);
}

bool UiActivityArt::place(const std::string& place, ::godot::Dictionary& out) const {
    ::godot::String symbol;
    return !place.empty() && read_text(places_, place.c_str(), symbol) &&
           art(std::string(symbol.utf8().get_data()), out);
}

}  // namespace ase::adp::godot
