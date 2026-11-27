/**
 * ASE GODOT ADAPTER - HUD ACTIVITY BARS BLOCK
 *
 * @file        godot_hud_activity_bars_draw.cpp
 * @brief       ActivityBars: binds hud.bars and the tokens hud_bars_*, draws the bars, names the action under a touch
 * @description MIG_ASE_SKILL_RSM step B5, see godot_hud_activity_bars_draw.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_074
 * @category    ecs/module
 * @usecase     usecase:101305 - show skill progress
 * @actor       Skill Measurement Tracker (SKILMEATRA)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.02.00002 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_activity_bars_draw.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/hud/ring/types.hpp>
#include <ase/adp/godot/rsm/types.hpp>

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

#include <algorithm>
#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

namespace {

/** Reads every token of `slots`; false with the first missing name in `failed`. */
bool read_tokens(const UiTheme& theme, std::initializer_list<std::pair<const char*, double*>> slots,
                 std::string& failed) {
    for (const std::pair<const char*, double*>& slot : slots) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    return true;
}

/** The items of hud.bars.`code`.items into `out`, each an item hud.items names; false with the bad one in `failed`. */
bool read_items(const UiTheme& theme, const ::godot::Dictionary& entry, const std::string& code,
                std::vector<std::string>& out, std::string& failed) {
    const ::godot::Variant value = entry.get("items", ::godot::Variant());
    if (value.get_type() != ::godot::Variant::ARRAY) {
        failed = "hud.bars." + code + ".items";
        return false;
    }
    const ::godot::Array list = value;
    out.clear();
    for (int64_t index = 0; index < list.size(); ++index) {
        const ::godot::Variant item = list[index];
        std::string widget;
        std::string id;
        if (item.get_type() == ::godot::Variant::STRING) {
            const ::godot::String text = item;
            id = utf8_of(text);
        }
        if (id.empty() || !theme.item_widget_class(id, widget)) {
            failed = "hud.bars." + code + ".items[" + std::to_string(index) + "]: no item of hud.items";
            return false;
        }
        out.push_back(id);
    }
    return true;
}

}  // namespace

bool ActivityBars::bind(const UiTheme& theme, const ::godot::Dictionary& root, std::string& failed) {
    double text_x = 0.0;
    double text_y = 0.0;
    if (!read_tokens(theme,
                     {{"hud_bars_x", &layout_.x}, {"hud_bars_base", &layout_.base}, {"hud_bars_w", &layout_.w},
                      {"hud_bars_gap", &layout_.gap}, {"hud_bars_unit", &layout_.unit}, {"hud_bars_h", &layout_.h},
                      {"hud_bars_hit_pad", &layout_.pad}, {"hud_bars_dim_alpha", &dim_}, {"hud_line", &line_},
                      {"hud_bars_text_x", &text_x}, {"hud_bars_text_y", &text_y},
                      {"hud_bars_text_size", &text_size_}},
                     failed)) {
        return false;
    }
    double band_text_x = 0.0;
    double band_text_y = 0.0;
    double switch_x = 0.0;
    double switch_y = 0.0;
    double switch_w = 0.0;
    double switch_h = 0.0;
    if (!read_tokens(theme,
                     {{"hud_band_bars_x", &band_layout_.x}, {"hud_band_bars_base", &band_layout_.base},
                      {"hud_band_bars_w", &band_layout_.w}, {"hud_band_bars_gap", &band_layout_.gap},
                      {"hud_band_bars_unit", &band_layout_.unit}, {"hud_band_bars_h", &band_layout_.h},
                      {"hud_band_bars_hit_pad", &band_layout_.pad}, {"hud_band_bars_text_x", &band_text_x},
                      {"hud_band_bars_text_y", &band_text_y}, {"hud_band_switch_x", &switch_x},
                      {"hud_band_switch_y", &switch_y}, {"hud_band_switch_w", &switch_w},
                      {"hud_band_switch_h", &switch_h}, {"hud_band_switch_text_size", &switch_text_size_},
                      {"hud_band_switch_text_dy", &switch_text_dy_}, {"hud_band_switch_alpha", &switch_alpha_}},
                     failed)) {
        return false;
    }
    if (!theme.color("OVERLAY_INK", ink_)) {
        failed = "OVERLAY_INK";
        return false;
    }
    if (!theme.color("OVERLAY_SHADOW", ground_)) {
        failed = "OVERLAY_SHADOW";
        return false;
    }
    ground_.a = static_cast<float>(switch_alpha_);
    text_at_ = ::godot::Vector2(static_cast<float>(text_x), static_cast<float>(text_y));
    band_text_at_ = ::godot::Vector2(static_cast<float>(band_text_x), static_cast<float>(band_text_y));
    switch_ = ::godot::Rect2(static_cast<float>(switch_x), static_cast<float>(switch_y), static_cast<float>(switch_w),
                             static_cast<float>(switch_h));
    // The words of the switch, by GODOT_RSM_BAND_*: the content a tap turns the band to.
    words_.clear();
    words_.resize(2u);
    for (const std::pair<uint32_t, const char*>& word : std::initializer_list<std::pair<uint32_t, const char*>>{
             {GODOT_RSM_BAND_SLOTS, "band.slots"}, {GODOT_RSM_BAND_BARS, "band.bars"}}) {
        std::string text;
        if (!plain_word(theme, word.second, text, failed) || !words_[word.first].bind_label(theme, failed) ||
            !words_[word.first].set(text, failed)) {
            return false;
        }
    }

    // hud.bars: one entry per action code BDI_CONST_ACTION_*, its domain tone and the tools whose holding lights it.
    ::godot::Dictionary hud;
    ::godot::Dictionary bars;
    if (!read_section(root, "hud", hud) || !read_section(hud, "bars", bars)) {
        failed = "hud.bars";
        return false;
    }
    actions_.clear();
    const ::godot::Array keys = bars.keys();
    for (int64_t index = 0; index < keys.size(); ++index) {
        const ::godot::String key = keys[index];
        if (key.begins_with("_")) {
            continue;
        }
        const std::string code = utf8_of(key);
        ::godot::Dictionary entry;
        ::godot::String tone;
        ActivityBarsAction action;
        if (!key.is_valid_int() || key.to_int() < 0 || key.to_int() >= static_cast<int64_t>(GODOT_RSM_ACTION_NONE)) {
            failed = "hud.bars." + code + ": no action code";
            return false;
        }
        if (!read_section(bars, code.c_str(), entry) || !read_text(entry, "tone", tone) ||
            !theme.tone(utf8_of(tone), action.tone)) {
            failed = "hud.bars." + code + ".tone";
            return false;
        }
        if (!read_items(theme, entry, code, action.items, failed)) {
            return false;
        }
        action.code = static_cast<uint32_t>(key.to_int());
        actions_.push_back(action);
    }
    if (actions_.empty()) {
        failed = "hud.bars: no action";
        return false;
    }
    if (!data_.bind_data(theme, failed)) {
        return false;
    }
    places_.clear();
    drawn_ = false;
    switch_drawn_ = false;
    drawn_layout_ = &layout_;
    return true;
}

void ActivityBars::step(const std::vector<ActivityBar>& bars, double wall, bool needs_slots) {
    activity_bars_order(bars, GODOT_RSM_BARS_MAX, ordered_);
    ActivityBandInput in;
    in.wall = wall;
    in.sign = activity_bars_sign(ordered_);
    in.count = ordered_.size();
    in.needs_slots = needs_slots;
    in.toggled = toggled_;
    activity_band_step(in, GODOT_RSM_BAND_HOLD_SECONDS, band_);
    toggled_ = false;
}

void ActivityBars::toggle() {
    toggled_ = true;
}

uint32_t ActivityBars::band() const {
    return band_.shows;
}

void ActivityBars::draw_toggle(UiPen& canvas, const ::godot::Transform2D& frame, uint32_t band) {
    // The band switch names the content a tap turns the band to: on the bars the slots, on the slots the bars.
    const uint32_t other = band == GODOT_RSM_BAND_BARS ? GODOT_RSM_BAND_SLOTS : GODOT_RSM_BAND_BARS;
    canvas.draw_set_transform_matrix(frame);
    canvas.draw_rect(switch_, ground_, true);
    canvas.draw_rect(switch_, ink_, false, static_cast<float>(line_));
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    UiTextPlace text;
    text.at = ::godot::Vector2(switch_.get_center().x, static_cast<float>(switch_.get_center().y + switch_text_dy_));
    text.size = switch_text_size_;
    text.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    text.color = ink_;
    text.opacity = 1.0;
    text.still = true;
    words_[other].draw(canvas, frame, text);
    switch_drawn_ = true;
}

bool ActivityBars::draw(UiPen& canvas, const ::godot::Transform2D& frame, const ActivityBarsFrame& now,
                        std::string& failed) {
    drawn_ = false;
    switch_drawn_ = false;
    const bool band = band_.shows == GODOT_RSM_BAND_BARS;
    drawn_layout_ = band ? &band_layout_ : &layout_;
    activity_bars_order(now.bars, GODOT_RSM_BARS_MAX, ordered_);
    // The lit actions: the bar a finger holds, else every action whose tool is in the hand.
    lit_.clear();
    if (now.held != GODOT_RSM_ACTION_NONE) {
        lit_.push_back(now.held);
    } else if (!now.hand.empty()) {
        for (const ActivityBarsAction& action : actions_) {
            if (std::find(action.items.begin(), action.items.end(), now.hand) != action.items.end()) {
                lit_.push_back(action.code);
            }
        }
    }
    activity_bars_place(ordered_, *drawn_layout_, lit_, places_);
    // THE ROW STANDS ALWAYS, as the volume bars of the prototype (VolumeBarManager): the foot dashes of the
    // GODOT_RSM_BARS_MAX places stand before the first slice of the led figure too, so the row is seen; a bar grows
    // on its dash once the port names its slice. The switch stands only beside bars: without one the band has
    // nothing to turn to (activity_band_step).
    if (!places_.empty()) {
        draw_toggle(canvas, frame, band_.shows);
    }
    const ActivityBarsLayout& row = *drawn_layout_;
    canvas.draw_set_transform_matrix(frame);
    // one flat dash per place, as an empty volume bar of the prototype stands: the row reads as its 20 places
    for (uint32_t slot = 0; slot < GODOT_RSM_BARS_MAX; ++slot) {
        const double left = row.x + static_cast<double>(slot) * (row.w + row.gap);
        canvas.draw_line(::godot::Vector2(static_cast<float>(left), static_cast<float>(row.base)),
                         ::godot::Vector2(static_cast<float>(left + row.w), static_cast<float>(row.base)), ink_,
                         static_cast<float>(line_));
    }
    if (places_.empty()) {
        canvas.draw_set_transform_matrix(::godot::Transform2D());
        return true;  // no slice of the led figure in the port: the foot line alone, no bar to touch
    }
    bool lit = false;
    for (const ActivityBarPlace& place : places_) {
        const auto found = std::find_if(actions_.begin(), actions_.end(),
                                        [&place](const ActivityBarsAction& action) { return action.code == place.action; });
        if (found == actions_.end()) {
            canvas.draw_set_transform_matrix(::godot::Transform2D());
            failed = "hud.bars." + std::to_string(place.action) + ": the action of slice " + std::to_string(place.id) +
                     " has no domain (BDI_CONST_ACTION_*)";
            return false;
        }
        ::godot::Color fill = found->tone;
        fill.a = static_cast<float>(place.lit ? 1.0 : dim_);
        const ::godot::Rect2 bar(static_cast<float>(place.x), static_cast<float>(place.y), static_cast<float>(place.w),
                                 static_cast<float>(place.h));
        canvas.draw_rect(bar, fill, true);
        if (place.lit) {
            canvas.draw_rect(bar, ink_, false, static_cast<float>(line_));
            lit = true;
        }
    }
    // The sum of the lit bars in the data face, while a shown bar is lit.
    if (lit) {
        if (!data_.set(hud_decimals(activity_bars_lit_qnt(places_), 0), failed)) {
            canvas.draw_set_transform_matrix(::godot::Transform2D());
            return false;
        }
        UiTextPlace text;
        text.at = band ? band_text_at_ : text_at_;
        text.size = text_size_;
        text.anchor = GODOT_TEXT_ANCHOR_START;
        text.color = ink_;
        text.opacity = 1.0;
        text.still = true;
        data_.draw(canvas, frame, text);
    }
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    drawn_ = true;
    return true;
}

bool ActivityBars::hit(const ::godot::Vector2& point, uint32_t& action) const {
    size_t index = 0;
    if (!drawn_ || !activity_bars_hit(places_, *drawn_layout_, point.x, point.y, index)) {
        return false;
    }
    action = places_[index].action;
    return true;
}

bool ActivityBars::switch_hit(const ::godot::Vector2& point) const {
    return switch_drawn_ && switch_.has_point(point);
}

bool ActivityBars::place(::godot::Rect2& out) const {
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
    if (!drawn_ || !activity_bars_strip(*drawn_layout_, places_.size(), x, y, w, h)) {
        return false;
    }
    out = ::godot::Rect2(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w), static_cast<float>(h));
    return true;
}

bool ActivityBars::switch_place(::godot::Rect2& out) const {
    if (!switch_drawn_) {
        return false;
    }
    out = switch_;
    return true;
}

void ActivityBars::bars(std::vector<ActivityBarPlace>& out) const {
    out.clear();
    if (drawn_) {
        out = places_;
    }
}

}  // namespace ase::adp::godot
