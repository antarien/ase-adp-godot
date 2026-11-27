/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW DEBUG MODE
 *
 * @file        godot_vivarium_debug.cpp
 * @brief       VivariumView - the debug buttons: the time lapse, the jump to a stage or an hour, the relief, the date entry
 * @description Operator order of 2026-10-26: the birch grows over game years, and its life course must be seen
 *              up to the adult tree without changing any server time. Launched with modes.debug.argument the
 *              view draws the buttons of the section "debug" in their own colours below the info lines, unlike
 *              every game button. Without the argument nothing of this is drawn or answered.
 *
 *              TWO CASES OF TIME, NOTHING ELSE (operator order of 2026-11-06):
 *
 *              THE TIME LAPSE (debug.buttons) runs this host's clock at a multiple of ase-time's base pace. A
 *              button names its factor only; the view sends it through the pace port, ase-pl-flora multiplies
 *              it with the base pace it reads from the star (FloraHostPaceSystem, usecase:101120 clock the growth
 *              cycle) and files the product as TIM_REQ_TIME_SCALE for ase-time. The view carries no constant of
 *              ase-time: what a game second is stands in ase-time's design and on the star, never here. The
 *              lapse has no target and no end; it holds until another factor is asked.
 *
 *              THE JUMP (debug.jumps) sets the clock to a target: a stage of the selected patch at the game hour
 *              ase-plant forecasts for it, an hour of the game day, a SURA date whole VECTUM ahead, the maximum
 *              of the next eclipse of the sun or of the moon, or the next rise of a moon that ase-ephemeris
 *              forecasts. All of them are the same case - only the recognition of the target differs, and every
 *              one is ONE write into ase-time's clock (CAUSA_ASE_TIME, section "The Seek Contract"): through the
 *              seek port, or for a date through the date port and ase-calendar. Nothing is replayed and no time is
 *              fed beyond the frame's own; every module computes its state at the new time in its own ticks. This
 *              unit reads, draws and dispatches the jump buttons; the ask, the start screen of the jump and its end
 *              stand in godot_vivarium_jump.cpp (VivariumView::ask_jump, ::seek_jump, ::follow_debug), the share
 *              the start screen shows in godot_vivarium_clock.cpp. The pace stays as it was before the jump - the
 *              jump never touches the pace port.
 *
 *              THE FREE DATE ENTRY (debug.date, operator order of 2026-11-08) jumps to ANY SURA date, ahead or
 *              back, beside the fixed jump buttons: fields of VECTUM, LUNBREX, TUMBRAE and HORAX, steps that move
 *              them, go and now. This unit reads the section through VivariumView::load_debug_date, hands a tap
 *              its buttons miss to VivariumView::date_tap and draws the entry with VivariumView::draw_debug_date;
 *              all three stand in godot_vivarium_date.cpp.
 *
 *              THE RELIEF BUTTONS (debug.reliefs, usecase:6018 derive terrain heights) change the relief of the
 *              whole ground at runtime. A button asks the elevation port for its height amplitude and ground level
 *              (GodotHostResourceManager::shape); ase-pl-flora writes them as TER_GEN_HGT_SCL and TER_GEN_BAS_HGT
 *              on the star, ase-terrain moves every generated chunk onto the new profile and lays the brook beds
 *              and the flattened patches anew. The seed is not touched, so the landscape keeps its shape and only
 *              its height changes.
 *
 *              THE ICONS OF THE BUTTONS are never raw glyphs of the configuration: a button names an ICON_*,
 *              the view resolves it in SHA's icon table (debug.icon_table) and draws it in the Nerd Font
 *              Icon.tsx draws with (debug.icon_font), shifted by the offset Icon.tsx gives it
 *              (godot_view_support.hpp draw_icon_label). A name the table lacks, or a glyph the font lacks,
 *              refuses the configuration. A jump may name no icon; it is drawn with its label alone.
 *
 *              The running factor, the running jump and the standing relief are drawn with the wider outline.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066, DSGN_017
 * @category    ecs/module
 * @usecase     usecase:100390 - return to the garden
 * @usecase     usecase:6018 - derive terrain heights
 * @actor       Plant Keeper (PLNTKEEPER), Terrain Height Shaper (TERRHEISHA)
 * @created     2026-10-26
 * @modified    2026-11-19
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_view_layout.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/hud/log/types.hpp>
#include <ase/adp/godot/hud/style/types.hpp>
#include <ase/adp/godot/patch/types.hpp>
#include <ase/kernel/kernel_types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/variant/array.hpp>

#include <cstdint>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

bool VivariumView::load_debug(const ::godot::Dictionary& root) {
    // The section stands in every configuration and is read always - a broken debug section is a broken
    // configuration on a normal start too; it is drawn and answered only with the launch argument.
    debug_ = VivariumDebug{};
    ::godot::Dictionary debug;
    ::godot::String fill;
    ::godot::String text;
    if (!read_section(root, "debug", debug) || !read_text(debug, "fill", fill) || !read_text(debug, "text", text) ||
        !fill.is_valid_html_color() || !text.is_valid_html_color()) {
        return fail_config("debug");
    }
    debug_.fill = ::godot::Color::html(fill);
    debug_.text = ::godot::Color::html(text);
    // THE ICONS OF THE BUTTONS come from SHA's icon SSOT, never as a raw glyph of this file: the table
    // names every ICON_* with its codepoint and the offset Icon.tsx gives it, the font is the Nerd Font
    // Icon.tsx draws with (godot_view_support.hpp). Both are staged beside this configuration.
    ::godot::String icon_table;
    ::godot::String icon_font;
    ::godot::Dictionary icons;
    if (!read_text(debug, "icon_table", icon_table) ||
        !read_icon_table(icon_table, icons, debug_.icon_web_px)) {
        return fail_config("debug.icon_table");
    }
    if (!read_text(debug, "icon_font", icon_font)) {
        return fail_config("debug.icon_font");
    }
    debug_.icon_font = icon_font_of(icon_font);
    if (debug_.icon_font.is_null()) {
        return fail_config("debug.icon_font");
    }
    // THE SETTLE OF A JUMP: the simulated seconds the host runs at the new time after the one write, before
    // follow_debug judges the landing. Above 0; the jump hands the host no second beyond the frame's own.
    if (!read_number(debug, "settle_s", debug_.settle_s) || !(debug_.settle_s > 0.0)) {
        return fail_config("debug.settle_s");
    }
    // THE SEARCH ON OF AN ECLIPSE JUMP (D2256, CAUSA_ASE_EPHEMERIS EphemerisEclNextSystem): a kind without an
    // eclipse inside ase-ephemeris' horizon carries the end of the horizon, and a reader that jumps by it
    // searches on from there. eclipse_hops is how often the jump does so before it notes no_eclipse; whole, 1
    // or above.
    double eclipse_hops = 0.0;
    if (!read_number(debug, "eclipse_hops", eclipse_hops) || !(eclipse_hops >= 1.0) ||
        ase::math::floor(static_cast<float>(eclipse_hops)) != static_cast<float>(eclipse_hops)) {
        return fail_config("debug.eclipse_hops");
    }
    debug_.eclipse_hops = static_cast<uint32_t>(eclipse_hops);
    // THE TIME LAPSE BUTTONS: a factor of the base pace, 0 or above; 0 gives the default pace back.
    const ::godot::Variant buttons_value = debug.get("buttons", ::godot::Variant());
    if (buttons_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("debug.buttons");
    }
    const ::godot::Array buttons = buttons_value;
    for (int64_t i = 0; i < buttons.size(); ++i) {
        if (buttons[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("debug.buttons[]");
        }
        const ::godot::Dictionary entry = buttons[i];
        VivariumDebugButton button;
        ::godot::String icon_name;
        if (!read_rect(entry, "rect", button.rect) || !read_text(entry, "label", button.label) ||
            !read_number(entry, "factor", button.factor) || !(button.factor >= GODOT_PACE_NONE)) {
            return fail_config("debug.buttons[]");
        }
        if (!read_text(entry, "icon", icon_name) ||
            !read_icon(icons, icon_name, debug_.icon_font, button.icon, button.icon_offset)) {
            return fail_config("debug.buttons[].icon");
        }
        if (!in_one_band(layout_.bands, button.rect.position.y, button.rect.get_end().y)) {
            return fail_config("layout.bands: debug.buttons[].rect");
        }
        debug_.buttons.push_back(button);
    }
    // THE JUMP BUTTONS: each names exactly one target, a stage of the catalogue or an hour of the game day,
    // 0 or above. How long a day is stands in ase-time, never in this view. The icon is optional.
    const ::godot::Variant jumps_value = debug.get("jumps", ::godot::Variant());
    if (jumps_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("debug.jumps");
    }
    const ::godot::Array jumps = jumps_value;
    for (int64_t i = 0; i < jumps.size(); ++i) {
        if (jumps[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("debug.jumps[]");
        }
        const ::godot::Dictionary entry = jumps[i];
        VivariumDebugJump jump;
        if (!read_rect(entry, "rect", jump.rect) || !read_text(entry, "label", jump.label)) {
            return fail_config("debug.jumps[]");
        }
        const bool names_stage = entry.has("stage");
        const bool names_hour = entry.has("hour");
        const bool names_years = entry.has("years");
        const bool names_eclipse = entry.has("eclipse");
        const bool names_moonrise = entry.has("moonrise");
        const bool names_event = entry.has("event");
        const uint32_t named = (names_stage ? 1u : 0u) + (names_hour ? 1u : 0u) + (names_years ? 1u : 0u) +
                               (names_eclipse ? 1u : 0u) + (names_moonrise ? 1u : 0u) + (names_event ? 1u : 0u);
        if (named != 1u) {
            return fail_config("debug.jumps[]: stage, hour, years, eclipse, moonrise or event");
        }
        if (names_event) {
            // AN EVENT JUMP names an ase-calendar event kind by its word (fullmoon, eclipse, ...), never its code:
            // the code stands on ase-calendar's code record of that kind (flora.event_code.v1, key event_<word>),
            // and ase-calendar finds the next occurrence and moves the clock (seek_event on flora.event.v1).
            if (!read_text(entry, "event", jump.event) || jump.event.is_empty()) {
                return fail_config("debug.jumps[].event");
            }
        } else if (names_moonrise) {
            // A MOONRISE JUMP carries no number: it lands at ase-ephemeris' forecast of the next rise of a moon.
            const ::godot::Variant moonrise_value = entry.get("moonrise", ::godot::Variant());
            if (moonrise_value.get_type() != ::godot::Variant::BOOL || !static_cast<bool>(moonrise_value)) {
                return fail_config("debug.jumps[].moonrise");
            }
            jump.moonrise = true;
        } else if (names_eclipse) {
            // AN ECLIPSE JUMP names the range of ase-ephemeris ECLIPSE_* kinds it lands on, [low, high],
            // whole, 1 to GODOT_ECLIPSE_KIND_MAX and low not above high: [1, 2] the sun, [3, 4] the moon.
            const ::godot::Variant range_value = entry.get("eclipse", ::godot::Variant());
            if (range_value.get_type() != ::godot::Variant::ARRAY) {
                return fail_config("debug.jumps[].eclipse");
            }
            const ::godot::Array range = range_value;
            if (range.size() != 2) {
                return fail_config("debug.jumps[].eclipse");
            }
            const ::godot::Variant::Type low_type = range[0].get_type();
            const ::godot::Variant::Type high_type = range[1].get_type();
            if ((low_type != ::godot::Variant::INT && low_type != ::godot::Variant::FLOAT) ||
                (high_type != ::godot::Variant::INT && high_type != ::godot::Variant::FLOAT)) {
                return fail_config("debug.jumps[].eclipse");
            }
            const double low = static_cast<double>(range[0]);
            const double high = static_cast<double>(range[1]);
            const double kind_max = static_cast<double>(GODOT_ECLIPSE_KIND_MAX);
            if (!(low >= 1.0) || !(high >= low) || high > kind_max ||
                ase::math::floor(static_cast<float>(low)) != static_cast<float>(low) ||
                ase::math::floor(static_cast<float>(high)) != static_cast<float>(high)) {
                return fail_config("debug.jumps[].eclipse");
            }
            jump.eclipse_low = static_cast<uint8_t>(low);
            jump.eclipse_high = static_cast<uint8_t>(high);
        } else if (names_stage) {
            double stage = 0.0;
            if (!read_number(entry, "stage", stage) || !(stage >= 0.0) ||
                stage >= static_cast<double>(GODOT_STAGE_COUNT) ||
                ase::math::floor(static_cast<float>(stage)) != static_cast<float>(stage)) {
                return fail_config("debug.jumps[].stage");
            }
            jump.stage = static_cast<uint8_t>(stage);
        } else if (names_years) {
            // A DATE JUMP names whole VECTUM ahead of the running one, 1 or more.
            if (!read_number(entry, "years", jump.years) || !(jump.years >= 1.0) ||
                ase::math::floor(static_cast<float>(jump.years)) != static_cast<float>(jump.years)) {
                return fail_config("debug.jumps[].years");
            }
        } else if (!read_number(entry, "hour", jump.hour) || !(jump.hour >= 0.0)) {
            return fail_config("debug.jumps[].hour");
        }
        if (entry.has("icon")) {
            ::godot::String icon_name;
            if (!read_text(entry, "icon", icon_name) ||
                !read_icon(icons, icon_name, debug_.icon_font, jump.icon, jump.icon_offset)) {
                return fail_config("debug.jumps[].icon");
            }
        }
        if (!in_one_band(layout_.bands, jump.rect.position.y, jump.rect.get_end().y)) {
            return fail_config("layout.bands: debug.jumps[].rect");
        }
        debug_.jumps.push_back(jump);
    }
    // The relief buttons: a height amplitude above 0 and a ground level, both in metres. The section is
    // required like the buttons - a configuration without it names no relief, an empty array is allowed.
    const ::godot::Variant reliefs_value = debug.get("reliefs", ::godot::Variant());
    if (reliefs_value.get_type() != ::godot::Variant::ARRAY) {
        return fail_config("debug.reliefs");
    }
    const ::godot::Array reliefs = reliefs_value;
    for (int64_t i = 0; i < reliefs.size(); ++i) {
        if (reliefs[i].get_type() != ::godot::Variant::DICTIONARY) {
            return fail_config("debug.reliefs[]");
        }
        const ::godot::Dictionary entry = reliefs[i];
        VivariumDebugRelief relief;
        if (!read_rect(entry, "rect", relief.rect) || !read_text(entry, "label", relief.label) ||
            !read_number(entry, "height_scale_m", relief.height_scale_m) || !(relief.height_scale_m > 0.0) ||
            !read_number(entry, "base_height_m", relief.base_height_m)) {
            return fail_config("debug.reliefs[]");
        }
        if (!in_one_band(layout_.bands, relief.rect.position.y, relief.rect.get_end().y)) {
            return fail_config("layout.bands: debug.reliefs[].rect");
        }
        debug_.reliefs.push_back(relief);
    }
    // THE FREE DATE ENTRY (debug.date) is read, answered and drawn by godot_vivarium_date.cpp.
    if (!load_debug_date(debug)) {
        return false;
    }
    debug_.on = launched_with(modes_.debug_argument);
    return true;
}

bool VivariumView::debug_tap(const ::godot::Vector2& at) {
    if (!debug_.on) {
        return false;
    }
    for (const VivariumDebugButton& button : debug_.buttons) {
        if (placed(button.rect).has_point(at)) {
            ask_pace(button);
            return true;
        }
    }
    for (uint32_t index = 0u; index < static_cast<uint32_t>(debug_.jumps.size()); ++index) {
        if (placed(debug_.jumps[index].rect).has_point(at)) {
            ask_jump(debug_.jumps[index], index);
            return true;
        }
    }
    for (const VivariumDebugRelief& relief : debug_.reliefs) {
        if (placed(relief.rect).has_point(at)) {
            ask_shape(relief);
            return true;
        }
    }
    return date_tap(at);
}

void VivariumView::ask_pace(const VivariumDebugButton& button) {
    // THE TIME LAPSE: the factor alone goes through the pace port; the base pace and the product are
    // ase-time's, multiplied on the star. A running jump goes on - it rides whatever pace stands.
    const HostStatus status = resources_.pace(button.factor);
    note("debug pace factor=" + ::godot::String::num(button.factor, GODOT_LOG_DECIMALS) + " result=" +
         GodotHostResourceManager::status_name(status));
    if (status != ase::kernel::HostStatusOk) {
        return;
    }
    debug_.factor = button.factor;
    node_->queue_redraw();
}

void VivariumView::ask_shape(const VivariumDebugRelief& relief) {
    // The relief goes through the elevation port of the host, the same port the start configuration uses
    // for the ground: ase-pl-flora files it on the star, ase-terrain derives the heights from it.
    const HostStatus status = resources_.shape(relief.height_scale_m, relief.base_height_m);
    note("debug shape height_scale_m=" + ::godot::String::num(relief.height_scale_m, GODOT_LOG_DECIMALS) +
         " base_height_m=" + ::godot::String::num(relief.base_height_m, GODOT_LOG_DECIMALS) + " result=" +
         GodotHostResourceManager::status_name(status));
    if (status != ase::kernel::HostStatusOk) {
        return;
    }
    debug_.height_scale_m = relief.height_scale_m;
    node_->queue_redraw();
}

void VivariumView::draw_debug() {
    if (!debug_.on) {
        return;
    }
    // THE DEBUG BUTTONS LOOK LIKE NONE OF THE GAME: their own face and text colour, the small font, a wider
    // outline on the button whose factor runs, on the jump that runs and on the relief button whose relief
    // stands.
    const ::godot::Ref<::godot::Font> font = view_font();
    const float text_lift = (font->get_ascent(GODOT_FONT_SMALL) - font->get_descent(GODOT_FONT_SMALL)) * GODOT_HALF;
    for (const VivariumDebugButton& button : debug_.buttons) {
        use_band(button.rect.position.y);
        const bool running = debug_.factor > GODOT_PACE_NONE && button.factor == debug_.factor;
        node_->draw_rect(button.rect, debug_.fill);
        node_->draw_rect(button.rect, debug_.text, false, running ? GODOT_BORDER_WIDTH_ACTIVE : GODOT_BORDER_WIDTH);
        draw_icon_label(*node_, debug_.icon_font, button.rect, button.icon, button.icon_offset, debug_.icon_web_px,
                        button.label, GODOT_FONT_SMALL, debug_.text);
    }
    for (uint32_t index = 0u; index < static_cast<uint32_t>(debug_.jumps.size()); ++index) {
        const VivariumDebugJump& jump = debug_.jumps[index];
        use_band(jump.rect.position.y);
        const bool running = debug_.jump.running && debug_.jump.button == index;
        node_->draw_rect(jump.rect, debug_.fill);
        node_->draw_rect(jump.rect, debug_.text, false, running ? GODOT_BORDER_WIDTH_ACTIVE : GODOT_BORDER_WIDTH);
        if (!jump.icon.is_empty()) {
            draw_icon_label(*node_, debug_.icon_font, jump.rect, jump.icon, jump.icon_offset, debug_.icon_web_px,
                            jump.label, GODOT_FONT_SMALL, debug_.text);
            continue;
        }
        node_->draw_string(font, ::godot::Vector2(jump.rect.position.x, jump.rect.get_center().y + text_lift),
                           jump.label, ::godot::HORIZONTAL_ALIGNMENT_CENTER, jump.rect.size.x, GODOT_FONT_SMALL,
                           debug_.text);
    }
    for (const VivariumDebugRelief& relief : debug_.reliefs) {
        use_band(relief.rect.position.y);
        const bool standing = debug_.height_scale_m > 0.0 && relief.height_scale_m == debug_.height_scale_m;
        node_->draw_rect(relief.rect, debug_.fill);
        node_->draw_rect(relief.rect, debug_.text, false, standing ? GODOT_BORDER_WIDTH_ACTIVE : GODOT_BORDER_WIDTH);
        node_->draw_string(font, ::godot::Vector2(relief.rect.position.x, relief.rect.get_center().y + text_lift),
                           relief.label, ::godot::HORIZONTAL_ALIGNMENT_CENTER, relief.rect.size.x, GODOT_FONT_SMALL,
                           debug_.text);
    }
    draw_debug_date(text_lift);
}

}  // namespace ase::adp::godot
