/**
 * ASE GODOT ADAPTER - HUD THEME
 *
 * @file        godot_hud_theme.cpp
 * @brief       Every colour, stroke, radius, gap and duration the HUD draws with, and its chips, read from `hud`
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.1, see godot_hud_theme.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-09
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_hud_theme.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_support.hpp>

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/** A colour as #RRGGBB or #RRGGBBAA under `key`. */
bool read_color(const ::godot::Dictionary& from, const char* key, ::godot::Color& out) {
    ::godot::String text;
    if (!read_text(from, key, text) || !text.is_valid_html_color()) {
        return false;
    }
    out = ::godot::Color::html(text);
    return true;
}

/** A size above 0 under `key`. */
bool read_size(const ::godot::Dictionary& from, const char* key, double& out) {
    return read_number(from, key, out) && out > 0.0;
}

/** A boolean under `key`. */
bool read_flag(const ::godot::Dictionary& from, const char* key, bool& out) {
    const ::godot::Variant value = from.get(key, ::godot::Variant());
    if (value.get_type() != ::godot::Variant::BOOL) {
        return false;
    }
    out = value;
    return true;
}

}  // namespace

bool read_hud_theme(const ::godot::Dictionary& root, const ::godot::Dictionary& icons,
                    const ::godot::Ref<::godot::FontFile>& icon_font, double icon_web_px, HudTheme& theme,
                    const char*& failed_key) {
    theme = HudTheme{};
    ::godot::Dictionary hud;
    if (!read_section(root, "hud", hud)) {
        failed_key = "hud";
        return false;
    }
    if (!read_color(hud, "ink", theme.ink) || !read_color(hud, "ink_mid", theme.ink_mid) ||
        !read_color(hud, "ink_low", theme.ink_low) || !read_color(hud, "shade", theme.shade)) {
        failed_key = "hud: ink, ink_mid, ink_low, shade";
        return false;
    }
    if (icon_font.is_null() || !(icon_web_px > 0.0)) {
        failed_key = "debug.icon_font, debug.icon_table";
        return false;
    }
    theme.icon_font = icon_font;
    theme.icon_web_px = icon_web_px;
    if (!read_size(hud, "line_px", theme.line_px) || !read_size(hud, "line_on_px", theme.line_on_px) ||
        !read_size(hud, "ring_radius_px", theme.ring_radius_px) ||
        !read_size(hud, "chip_radius_px", theme.chip_radius_px) ||
        !read_size(hud, "chip_gap_px", theme.chip_gap_px) || !read_size(hud, "spine_x_px", theme.spine_x_px) ||
        !read_size(hud, "spine_top_px", theme.spine_top_px) || !read_size(hud, "leader_px", theme.leader_px) ||
        !read_size(hud, "rail_x_px", theme.rail_x_px) || !read_size(hud, "rail_dot_px", theme.rail_dot_px) ||
        !read_size(hud, "label_px", theme.label_px) || !read_size(hud, "toast_s", theme.toast_s)) {
        failed_key = "hud: a size above 0";
        return false;
    }
    ::godot::Dictionary tones;
    ::godot::Dictionary buttons;
    if (!read_section(hud, "tones", tones)) {
        failed_key = "hud.tones";
        return false;
    }
    if (!read_section(root, "buttons", buttons)) {
        failed_key = "buttons";
        return false;
    }
    const ::godot::Variant chips_value = hud.get("chips", ::godot::Variant());
    if (chips_value.get_type() != ::godot::Variant::ARRAY) {
        failed_key = "hud.chips";
        return false;
    }
    const ::godot::Array chips = chips_value;
    for (int64_t i = 0; i < chips.size(); ++i) {
        if (chips[i].get_type() != ::godot::Variant::DICTIONARY) {
            failed_key = "hud.chips[]";
            return false;
        }
        const ::godot::Dictionary entry = chips[i];
        HudChip chip;
        HudChipRule rule;
        ::godot::String icon_name;
        ::godot::String tone_name;
        ::godot::Dictionary button;
        if (!read_text(entry, "id", chip.id) || !read_flag(entry, "needs_place", rule.needs_place) ||
            !read_flag(entry, "debug_only", rule.debug_only)) {
            failed_key = "hud.chips[]: id, needs_place, debug_only";
            return false;
        }
        if (!read_section(buttons, utf8_of(chip.id).c_str(), button) || !read_text(button, "label", chip.label)) {
            failed_key = "hud.chips[].id: a key of buttons";
            return false;
        }
        if (!read_text(entry, "icon", icon_name) ||
            !read_icon(icons, icon_name, icon_font, chip.icon, chip.icon_offset)) {
            failed_key = "hud.chips[].icon";
            return false;
        }
        if (!read_text(entry, "tone", tone_name) || !read_color(tones, utf8_of(tone_name).c_str(), chip.tone)) {
            failed_key = "hud.chips[].tone: a key of hud.tones";
            return false;
        }
        theme.chips.push_back(chip);
        theme.rules.push_back(rule);
    }
    return true;
}

}  // namespace ase::adp::godot
