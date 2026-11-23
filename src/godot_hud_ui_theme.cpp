/**
 * ASE GODOT ADAPTER - HUD UI THEME
 *
 * @file        godot_hud_ui_theme.cpp
 * @brief       Reads SHA's colours, tokens, words and fonts, the zones and the items once, and answers lookups by name
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.1, see godot_hud_ui_theme.hpp. The brain rules of the
 *              section hud are read by UiTheme::read_spec (godot_hud_ui_theme_spec.cpp), which load calls last.
 *
 *              THE ITEMS HAVE TWO SOURCES AND ONE KEY. item_art.json of SHA carries the drawing of each item;
 *              hud.items of the start configuration names, under the same key, the widget class of the plan
 *              (DibberTool, WateringPot, ...), the mode the item puts the hand in and its ground verbs.
 *              item_widget answers a widget class with its item, so a class learns its item from the file
 *              and no unit writes an item key.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_support.hpp>

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <cstdint>

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

/** A face of hud.fonts, loaded at run time. */
bool read_face(const ::godot::Dictionary& fonts, const char* key, ::godot::Ref<::godot::FontFile>& out) {
    ::godot::String path;
    if (!read_text(fonts, key, path)) {
        return false;
    }
    out = icon_font_of(path);
    return out.is_valid();
}

/**
 * Every character the text runs of `words` show, each once in the order first met (UiTheme::alphabet). A {name}
 * of a port value is left out with its braces: fill_words writes the value there, never the name.
 */
::godot::String alphabet_of(const ::godot::Dictionary& words) {
    ::godot::String alphabet;
    const ::godot::Array keys = words.keys();
    for (int64_t key = 0; key < keys.size(); ++key) {
        const ::godot::Variant value = words.get(keys[key], ::godot::Variant());
        if (value.get_type() != ::godot::Variant::ARRAY) {
            continue;
        }
        const ::godot::Array runs = value;
        for (int64_t index = 0; index < runs.size(); ++index) {
            const ::godot::Variant item = runs[index];
            if (item.get_type() != ::godot::Variant::DICTIONARY) {
                continue;
            }
            const ::godot::Dictionary run = item;
            ::godot::String text;
            if (!read_text(run, "text", text)) {
                continue;
            }
            bool port = false;
            for (int64_t at = 0; at < text.length(); ++at) {
                const ::godot::String one = text.substr(at, 1);
                if (one == "{") {
                    port = true;
                } else if (one == "}") {
                    port = false;
                } else if (!port && alphabet.find(one) < 0) {
                    alphabet += one;
                }
            }
        }
    }
    return alphabet;
}

}  // namespace

bool UiTheme::load(const ::godot::Dictionary& root, std::string& failed) {
    ::godot::Dictionary hud;
    if (!read_section(root, "hud", hud)) {
        failed = "hud";
        return false;
    }

    // The language of the words.
    ::godot::String language;
    if (!read_text(hud, "language", language)) {
        failed = "hud.language";
        return false;
    }
    language_ = utf8_of(language);

    // The four SHA files the start configuration names under hud.distribution.
    ::godot::Dictionary distribution;
    ::godot::String colors_path;
    ::godot::String tokens_path;
    ::godot::String words_path;
    ::godot::String items_path;
    if (!read_section(hud, "distribution", distribution)) {
        failed = "hud.distribution";
        return false;
    }
    ::godot::Dictionary colors_file;
    if (!read_text(distribution, "colors", colors_path) || !read_json_file(colors_path, colors_file) ||
        !read_section(colors_file, "colors", colors_)) {
        failed = "hud.distribution.colors";
        return false;
    }
    ::godot::Dictionary tokens_file;
    if (!read_text(distribution, "tokens", tokens_path) || !read_json_file(tokens_path, tokens_file) ||
        !read_section(tokens_file, "tokens", tokens_)) {
        failed = "hud.distribution.tokens";
        return false;
    }
    ::godot::Dictionary words_file;
    ::godot::Dictionary languages;
    ::godot::Dictionary entry;
    if (!read_text(distribution, "words", words_path) || !read_json_file(words_path, words_file) ||
        !read_section(words_file, "languages", languages) || !read_section(languages, language_.c_str(), entry) ||
        !read_section(entry, "keys", words_)) {
        failed = "hud.distribution.words (" + language_ + ")";
        return false;
    }
    alphabet_ = alphabet_of(words_);
    const ::godot::Variant drift = entry.get("drift", ::godot::Variant());
    if (drift.get_type() == ::godot::Variant::BOOL && static_cast<bool>(drift)) {
        // A translation older than its source: shown, and named, never passed off as current.
        ::godot::UtilityFunctions::push_warning(::godot::String("UiTheme: the words of ") + language +
                                                " were translated from an older source (drift) in " + words_path);
    }
    // The drawings of the carried things (item-art.json through generate-item-art.cjs).
    ::godot::Dictionary items_file;
    if (!read_text(distribution, "items", items_path) || !read_json_file(items_path, items_file) ||
        !read_section(items_file, "items", items_)) {
        failed = "hud.distribution.items";
        return false;
    }
    // The widget class of each item, the mode it puts the hand in and its ground verbs (the plan's table).
    if (!read_section(hud, "items", things_)) {
        failed = "hud.items";
        return false;
    }

    // The icons: SHA's table and the Nerd Font, from the section debug, the one path of both.
    ::godot::Dictionary debug;
    ::godot::String icon_table;
    if (!read_section(root, "debug", debug) || !read_text(debug, "icon_table", icon_table) ||
        !read_icon_table(icon_table, icons_, icon_web_px_)) {
        failed = "debug.icon_table";
        return false;
    }

    // The four faces.
    ::godot::Dictionary fonts;
    if (!read_section(hud, "fonts", fonts)) {
        failed = "hud.fonts";
        return false;
    }
    if (!read_face(fonts, "display_medium", faces_.display_medium)) {
        failed = "hud.fonts.display_medium";
        return false;
    }
    if (!read_face(fonts, "display_semibold", faces_.display_semibold)) {
        failed = "hud.fonts.display_semibold";
        return false;
    }
    if (!read_face(fonts, "body", faces_.body)) {
        failed = "hud.fonts.body";
        return false;
    }
    if (!read_face(fonts, "data", faces_.data)) {
        failed = "hud.fonts.data";
        return false;
    }
    // The letter spacing of the classes t and tb comes from SHA's tokens, like every other number of the HUD.
    if (!measure("hud_tracking_label", faces_.spacing_medium_em)) {
        failed = "hud_tracking_label";
        return false;
    }
    if (!measure("hud_tracking_title", faces_.spacing_semibold_em)) {
        failed = "hud_tracking_title";
        return false;
    }

    // The zones.
    if (!read_section(hud, "zones", zones_)) {
        failed = "hud.zones";
        return false;
    }

    // The brain rules (godot_hud_ui_theme_spec.cpp).
    HudContextSpec spec;
    if (!read_spec(hud, spec, failed)) {
        return false;
    }
    spec_ = spec;
    return true;
}

bool UiTheme::color(const std::string& name, ::godot::Color& out) const {
    ::godot::Dictionary entry;
    double red = 0.0;
    double green = 0.0;
    double blue = 0.0;
    double alpha = 0.0;
    if (!read_section(colors_, name.c_str(), entry) || !read_number(entry, "r", red) ||
        !read_number(entry, "g", green) || !read_number(entry, "b", blue) || !read_number(entry, "a", alpha)) {
        return false;
    }
    // colors.json writes the channels as 0..255 and the opacity as 0..1, as colors.ts does.
    const double channel_max = 255.0;
    out = ::godot::Color(static_cast<float>(red / channel_max), static_cast<float>(green / channel_max),
                         static_cast<float>(blue / channel_max), static_cast<float>(alpha));
    return true;
}

bool UiTheme::tone(const std::string& name, ::godot::Color& out) const {
    if (name.empty()) {
        return false;
    }
    // The domain code of colors.ts (section 25): water is DOMAIN_WATER, flow DOMAIN_FLOW.
    const ::godot::String key = ::godot::String("DOMAIN_") + text_of(name).to_upper();
    return color(utf8_of(key), out);
}

bool UiTheme::item_art(const std::string& name, ::godot::Dictionary& out) const {
    return !name.empty() && read_section(items_, name.c_str(), out);
}

bool UiTheme::item_widget(const std::string& widget, std::string& id, std::string& mode,
                          std::vector<std::string>& verbs) const {
    const ::godot::Array keys = things_.keys();
    for (int64_t i = 0; i < keys.size(); ++i) {
        const ::godot::Variant entry_value = things_[keys[i]];
        if (entry_value.get_type() != ::godot::Variant::DICTIONARY) {
            continue;  // _description: a note, not an item
        }
        const ::godot::Dictionary entry = entry_value;
        ::godot::String named;
        ::godot::String mode_text;
        if (!read_text(entry, "widget", named) || utf8_of(named) != widget || !read_text(entry, "mode", mode_text)) {
            continue;
        }
        id = utf8_of(::godot::String(keys[i]));
        mode = utf8_of(mode_text);
        verbs.clear();
        // Only a ground tool names verbs; every other item has none (the plan's table).
        const ::godot::Variant list = entry.get("verbs", ::godot::Variant());
        if (list.get_type() == ::godot::Variant::ARRAY) {
            const ::godot::Array names = list;
            for (int64_t v = 0; v < names.size(); ++v) {
                verbs.push_back(utf8_of(::godot::String(names[v])));
            }
        }
        return true;
    }
    return false;
}

bool UiTheme::item_mode(const std::string& id, std::string& mode) const {
    ::godot::Dictionary entry;
    ::godot::String mode_text;
    if (!read_section(things_, id.c_str(), entry) || !read_text(entry, "mode", mode_text)) {
        return false;
    }
    mode = utf8_of(mode_text);
    return true;
}

bool UiTheme::item_thing(const std::string& id, std::string& tone, std::string& asset,
                         std::vector<std::string>& verbs, double& view_w, double& view_h) const {
    ::godot::Dictionary entry;
    ::godot::String tone_text;
    if (!read_section(things_, id.c_str(), entry) || !read_text(entry, "tone", tone_text) ||
        !entry.has("asset") || !read_number(entry, "vw", view_w) || !read_number(entry, "vh", view_h)) {
        return false;
    }
    // asset null: the item has no sprite of phase 04 yet and is drawn from its art alone (the seeds); any other
    // value is its asset's name.
    const ::godot::Variant asset_value = entry.get("asset", ::godot::Variant());
    if (asset_value.get_type() != ::godot::Variant::NIL && asset_value.get_type() != ::godot::Variant::STRING) {
        return false;
    }
    tone = utf8_of(tone_text);
    asset = asset_value.get_type() == ::godot::Variant::STRING ? utf8_of(::godot::String(asset_value)) : std::string();
    verbs.clear();
    // Only a ground tool names verbs; every other item has none (the plan's table).
    const ::godot::Variant list = entry.get("verbs", ::godot::Variant());
    if (list.get_type() == ::godot::Variant::ARRAY) {
        const ::godot::Array names = list;
        for (int64_t v = 0; v < names.size(); ++v) {
            verbs.push_back(utf8_of(::godot::String(names[v])));
        }
    }
    return true;
}

bool UiTheme::item_widget_class(const std::string& id, std::string& widget) const {
    ::godot::Dictionary entry;
    ::godot::String named;
    if (!read_section(things_, id.c_str(), entry) || !read_text(entry, "widget", named)) {
        return false;
    }
    widget = utf8_of(named);
    return true;
}

bool UiTheme::item_phase04(const std::string& id, std::string& phase04) const {
    ::godot::Dictionary entry;
    if (!read_section(things_, id.c_str(), entry) || !entry.has("asset")) {
        return false;
    }
    phase04.clear();
    if (entry.get("asset", ::godot::Variant()).get_type() != ::godot::Variant::NIL) {
        return true;  // drawn from its asset; D.asset || D.phase04 reads the asset
    }
    ::godot::String text;
    if (!read_text(entry, "phase04", text)) {
        return false;
    }
    phase04 = utf8_of(text);
    return true;
}

bool UiTheme::measure(const std::string& token, double& out) const {
    ::godot::Dictionary entry;
    return read_section(tokens_, token.c_str(), entry) && read_number(entry, "value", out);
}

bool UiTheme::token_text(const std::string& token, ::godot::String& out) const {
    // The class answers false only for a name the files do not carry: an empty text is carried, it is the part a
    // glyph does not have (hud_rail_state_*_fill, _line, _mark). A reader that needs a text refuses the empty one.
    ::godot::Dictionary entry;
    if (!read_section(tokens_, token.c_str(), entry)) {
        return false;
    }
    const ::godot::Variant value = entry.get("value", ::godot::Variant());
    if (value.get_type() != ::godot::Variant::STRING) {
        return false;
    }
    out = static_cast<::godot::String>(value);
    return true;
}

bool UiTheme::words(const std::string& key, std::vector<HudRun>& out) const {
    const ::godot::Variant value = words_.get(text_of(key), ::godot::Variant());
    if (value.get_type() != ::godot::Variant::ARRAY) {
        return false;
    }
    const ::godot::Array runs = value;
    out.clear();
    for (int64_t index = 0; index < runs.size(); ++index) {
        const ::godot::Variant item = runs[index];
        if (item.get_type() != ::godot::Variant::DICTIONARY) {
            return false;
        }
        const ::godot::Dictionary run = item;
        ::godot::String text;
        HudRun one;
        if (read_text(run, "icon", text)) {
            one.icon = utf8_of(text);
        } else if (read_text(run, "text", text)) {
            one.text = utf8_of(text);
        } else {
            return false;
        }
        out.push_back(one);
    }
    return !out.empty();
}

bool UiTheme::zone(const std::string& name, ::godot::Rect2& out) const {
    return read_rect(zones_, name.c_str(), out);
}

::godot::Rect2 hud_cover_dim(const HudCover& cover, double y, double h) {
    const double top = y + cover.head;
    const double bottom = y + h + cover.hand;
    return ::godot::Rect2(cover.surface.position.x, static_cast<float>(top), cover.surface.size.x,
                          static_cast<float>(bottom - top));
}

}  // namespace ase::adp::godot
