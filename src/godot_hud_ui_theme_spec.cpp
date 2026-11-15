/**
 * ASE GODOT ADAPTER - HUD UI THEME SPEC
 *
 * @file        godot_hud_ui_theme_spec.cpp
 * @brief       Reads the brain rules of the start configuration once: modes, layer rules, tools, fan, signs, perception
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.1, see godot_hud_ui_theme.hpp. UiTheme::load reads SHA's
 *              files and the faces (godot_hud_ui_theme.cpp) and hands the section hud to UiTheme::read_spec here:
 *              the rules the ContextResolver decides the visible set from (HudContextSpec), every one named in
 *              `failed` when the configuration does not carry it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_support.hpp>

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/** A key of the configuration that is a note, not a rule: it begins with an underscore. */
bool is_note(const ::godot::String& key) {
    return key.begins_with("_");
}

/** A list of non-empty texts under `key`; an empty list is a list. */
bool read_texts(const ::godot::Dictionary& from, const char* key, std::vector<std::string>& out) {
    const ::godot::Variant value = from.get(key, ::godot::Variant());
    if (value.get_type() != ::godot::Variant::ARRAY) {
        return false;
    }
    const ::godot::Array list = value;
    out.clear();
    for (int64_t index = 0; index < list.size(); ++index) {
        const ::godot::Variant item = list[index];
        if (item.get_type() != ::godot::Variant::STRING) {
            return false;
        }
        const ::godot::String text = item;
        if (text.is_empty()) {
            return false;
        }
        out.push_back(utf8_of(text));
    }
    return true;
}

/** A text under `key`, or empty where the JSON writes null. */
bool read_text_or_none(const ::godot::Dictionary& from, const char* key, std::string& out) {
    const ::godot::Variant value = from.get(key, ::godot::Variant());
    if (value.get_type() == ::godot::Variant::NIL && from.has(key)) {
        out.clear();
        return true;
    }
    ::godot::String text;
    if (!read_text(from, key, text)) {
        return false;
    }
    out = utf8_of(text);
    return true;
}

/** A whole number from 1 up under `key`. */
bool read_count(const ::godot::Dictionary& from, const char* key, uint32_t& out) {
    double number = 0.0;
    if (!read_number(from, key, number) || !(number >= 1.0) || number > static_cast<double>(UINT32_MAX)) {
        return false;
    }
    out = static_cast<uint32_t>(number);
    return static_cast<double>(out) == number;
}

/** The layer rules of hud.modes.over or hud.modes.hides: layer -> the marks that must all hold. */
bool read_layer_rules(const ::godot::Dictionary& modes, const char* key, std::vector<HudLayerRule>& out,
                      std::string& failed) {
    ::godot::Dictionary rules;
    if (!read_section(modes, key, rules)) {
        failed = std::string("hud.modes.") + key;
        return false;
    }
    out.clear();
    const ::godot::Array layers = rules.keys();
    for (int64_t index = 0; index < layers.size(); ++index) {
        const ::godot::String layer = layers[index];
        if (is_note(layer)) {
            continue;
        }
        HudLayerRule rule;
        rule.layer = utf8_of(layer);
        if (!read_texts(rules, rule.layer.c_str(), rule.marks) || rule.marks.empty()) {
            failed = std::string("hud.modes.") + key + "." + rule.layer;
            return false;
        }
        out.push_back(rule);
    }
    return true;
}

/**
 * The modes of hud.modes: every key that is neither a note, nor one of the four rules beside the modes,
 * nor the geometry of a hit area (tree_hit, trunk_hit: the stand of the birch and the trunk with its
 * pulse, read by the hit blocks).
 */
bool read_modes(const ::godot::Dictionary& modes, std::vector<HudModeSpec>& out, std::string& failed) {
    out.clear();
    const ::godot::Array names = modes.keys();
    for (int64_t index = 0; index < names.size(); ++index) {
        const ::godot::String name = names[index];
        if (is_note(name) || name == "over" || name == "hides" || name == "start" || name == "sign_lost" ||
            name == "tool_lost" || name.ends_with("_hit")) {
            continue;
        }
        HudModeSpec mode;
        mode.name = utf8_of(name);
        ::godot::Dictionary entry;
        if (!read_section(modes, mode.name.c_str(), entry) || !read_texts(entry, "hits", mode.hits) ||
            !read_texts(entry, "layers", mode.layers) || mode.layers.empty()) {
            failed = "hud.modes." + mode.name;
            return false;
        }
        out.push_back(mode);
    }
    if (out.empty()) {
        failed = "hud.modes";
        return false;
    }
    return true;
}

/** The tools of hud.tools. */
bool read_tools(const ::godot::Dictionary& hud, std::vector<HudToolSpec>& out, std::string& failed) {
    ::godot::Dictionary tools;
    if (!read_section(hud, "tools", tools)) {
        failed = "hud.tools";
        return false;
    }
    out.clear();
    const ::godot::Array names = tools.keys();
    for (int64_t index = 0; index < names.size(); ++index) {
        const ::godot::String name = names[index];
        if (is_note(name)) {
            continue;
        }
        HudToolSpec tool;
        tool.name = utf8_of(name);
        ::godot::Dictionary entry;
        if (!read_section(tools, tool.name.c_str(), entry) || !read_texts(entry, "requires", tool.marks_required) ||
            !read_text_or_none(entry, "has", tool.has)) {
            failed = "hud.tools." + tool.name;
            return false;
        }
        out.push_back(tool);
    }
    return true;
}

/** The signs of hud.signs, in the order the configuration writes them: the order of the rail. */
bool read_signs(const ::godot::Dictionary& hud, std::vector<HudSignSpec>& out, std::string& failed) {
    ::godot::Dictionary signs;
    if (!read_section(hud, "signs", signs)) {
        failed = "hud.signs";
        return false;
    }
    out.clear();
    const ::godot::Array names = signs.keys();
    for (int64_t index = 0; index < names.size(); ++index) {
        const ::godot::String name = names[index];
        if (is_note(name)) {
            continue;
        }
        HudSignSpec sign;
        sign.name = utf8_of(name);
        ::godot::Dictionary entry;
        if (!read_section(signs, sign.name.c_str(), entry) || !read_text_or_none(entry, "offer", sign.offer)) {
            failed = "hud.signs." + sign.name;
            return false;
        }
        // alert is true or false, or the mark while which the sign alerts.
        const ::godot::Variant alert = entry.get("alert", ::godot::Variant());
        if (alert.get_type() == ::godot::Variant::BOOL) {
            sign.alert = alert;
        } else if (!read_text_or_none(entry, "alert", sign.alert_when) || sign.alert_when.empty()) {
            failed = "hud.signs." + sign.name + ".alert";
            return false;
        }
        // offer_when stands only where an offer waits for marks.
        if (entry.has("offer_when") && !read_texts(entry, "offer_when", sign.offer_when)) {
            failed = "hud.signs." + sign.name + ".offer_when";
            return false;
        }
        out.push_back(sign);
    }
    return true;
}

/** hud.focus.fan: the tools in the order of the places, and how many places there are. */
bool read_fan(const ::godot::Dictionary& hud, HudContextSpec& spec, std::string& failed) {
    ::godot::Dictionary focus;
    ::godot::Dictionary fan;
    if (!read_section(hud, "focus", focus) || !read_section(focus, "fan", fan) ||
        !read_texts(fan, "tools", spec.fan)) {
        failed = "hud.focus.fan.tools";
        return false;
    }
    const ::godot::Variant offsets = fan.get("offsets", ::godot::Variant());
    if (offsets.get_type() != ::godot::Variant::ARRAY) {
        failed = "hud.focus.fan.offsets";
        return false;
    }
    const ::godot::Array places = offsets;
    spec.fan_places = static_cast<uint32_t>(places.size());
    return true;
}

/** hud.perception: the levels and the level the view starts at. */
bool read_perception(const ::godot::Dictionary& hud, HudContextSpec& spec, std::string& failed) {
    ::godot::Dictionary perception;
    if (!read_section(hud, "perception", perception)) {
        failed = "hud.perception";
        return false;
    }
    const ::godot::Variant levels = perception.get("levels", ::godot::Variant());
    if (levels.get_type() != ::godot::Variant::ARRAY) {
        failed = "hud.perception.levels";
        return false;
    }
    const ::godot::Array list = levels;
    spec.levels.clear();
    for (int64_t index = 0; index < list.size(); ++index) {
        ::godot::Dictionary one;
        one["level"] = list[index];
        uint32_t level = 0;
        if (!read_count(one, "level", level)) {
            failed = "hud.perception.levels";
            return false;
        }
        spec.levels.push_back(level);
    }
    if (!read_count(perception, "start", spec.level_start)) {
        failed = "hud.perception.start";
        return false;
    }
    return true;
}

}  // namespace

bool UiTheme::read_spec(const ::godot::Dictionary& hud, HudContextSpec& spec, std::string& failed) {
    ::godot::Dictionary modes;
    if (!read_section(hud, "modes", modes)) {
        failed = "hud.modes";
        return false;
    }
    if (!read_modes(modes, spec.modes, failed) || !read_layer_rules(modes, "over", spec.over, failed) ||
        !read_layer_rules(modes, "hides", spec.hides, failed)) {
        return false;
    }
    ::godot::String start;
    ::godot::String sign_lost;
    ::godot::String tool_lost;
    if (!read_text(modes, "start", start)) {
        failed = "hud.modes.start";
        return false;
    }
    if (!read_text(modes, "sign_lost", sign_lost)) {
        failed = "hud.modes.sign_lost";
        return false;
    }
    if (!read_text(modes, "tool_lost", tool_lost)) {
        failed = "hud.modes.tool_lost";
        return false;
    }
    spec.start = utf8_of(start);
    spec.sign_lost = utf8_of(sign_lost);
    spec.tool_lost = utf8_of(tool_lost);
    return read_tools(hud, spec.tools, failed) && read_fan(hud, spec, failed) &&
           read_signs(hud, spec.signs, failed) && read_perception(hud, spec, failed);
}

}  // namespace ase::adp::godot
