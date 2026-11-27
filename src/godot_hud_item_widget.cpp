/**
 * ASE GODOT ADAPTER - HUD ITEM WIDGET
 *
 * @file        godot_hud_item_widget.cpp
 * @brief       Binds an item widget class to its item and draws the thing, its count and its reading
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_item_widget.hpp. Every value is SHA's:
 *              the drawing from item_art.json, the tone and the hold from theme.css, the words from the yaml of
 *              the language; the item key comes from hud.items of the start configuration.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), N0457 (sap), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_item_widget.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/hud/drag/types.hpp>
#include <ase/adp/godot/hud/hand/types.hpp>
#include <ase/adp/godot/hud/ring/types.hpp>

#include <godot_cpp/variant/string.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool ItemWidget::bind(const UiTheme& theme, const char* widget, std::string& failed) {
    widget_ = widget;
    if (!theme.item_widget(widget_, id_, mode_, verbs_)) {
        failed = "hud.items: no item names the widget " + widget_;
        return false;
    }
    if (!sprite_.bind(theme, failed) || !sprite_.set(id_, failed)) {
        return false;
    }

    // The tone and the hold of the item (theme.css, --ase-hud-item-<id>-tone and -hold).
    const std::string tone_token = "hud_item_" + id_ + "_tone";
    const std::string hold_token = "hud_item_" + id_ + "_hold";
    ::godot::String tone_name;
    ::godot::String hold_name;
    if (!theme.token_text(tone_token, tone_name) || !theme.tone(utf8_of(tone_name), tone_)) {
        failed = tone_token;
        return false;
    }
    if (!theme.token_text(hold_token, hold_name)) {
        failed = hold_token;
        return false;
    }
    hold_.clear();
    const std::string hold_key = utf8_of(hold_name);
    if (hold_key != GODOT_HUD_ITEM_HOLD_NONE && !theme.words(hold_key, hold_)) {
        failed = "texts.hud." + hold_key;
        return false;
    }

    // The name and the case of the item.
    const std::string name_key = std::string(GODOT_HUD_WORD_ITEM_HEAD) + id_ + GODOT_HUD_WORD_ITEM_TAIL;
    const std::string case_key = std::string(GODOT_HUD_WORD_ITEM_HEAD) + id_ + GODOT_HUD_WORD_ITEM_CASE;
    if (!theme.words(name_key, name_)) {
        failed = "texts.hud." + name_key;
        return false;
    }
    if (!theme.words(case_key, case_)) {
        failed = "texts.hud." + case_key;
        return false;
    }

    for (const std::pair<const char*, double*>& slot :
         std::initializer_list<std::pair<const char*, double*>>{{"hud_item_count_dx", &count_dx_},
                                                                {"hud_item_count_dy", &count_dy_},
                                                                {"hud_item_count_size", &count_size_},
                                                                {"hud_item_volume_from", &volume_from_},
                                                                {"hud_load_decimals", &decimals_},
                                                                {"hud_load_fine_below", &fine_below_},
                                                                {"hud_load_fine_decimals", &fine_decimals_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{{"item_size", &size_word_},
                                                                             {"item_volume", &volume_word_},
                                                                             {"item_count", &count_word_},
                                                                             {"stack_count", &stack_word_}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }
    return count_text_.bind_data(theme, failed);
}

void ItemWidget::draw(UiPen& canvas, const ::godot::Transform2D& frame,
                      const UiItemSpritePlace& place) const {
    sprite_.draw(canvas, frame, place);
}

void ItemWidget::draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame,
                             const ::godot::Vector2& corner, double k, const UiItemSpritePlace& place) const {
    sprite_.draw_scaled(canvas, frame, corner, k, place);
}

bool ItemWidget::draw_count(UiPen& canvas, const ::godot::Transform2D& frame,
                            const UiItemSpritePlace& place, uint32_t count, const ::godot::Color& ink,
                            std::string& failed) {
    if (count <= 1) {
        return true;  // one thing names no count
    }
    std::vector<HudRun> runs = stack_word_;
    if (!fill_words(runs, {HudPortValue{"n", hud_whole(count)}}, failed) ||
        !count_text_.set(runs, failed)) {
        return false;
    }
    const double half = place.box / 2.0;
    UiTextPlace text;
    text.at = ::godot::Vector2(static_cast<float>(place.centre.x + half + count_dx_),
                               static_cast<float>(place.centre.y + half + count_dy_));
    text.size = count_size_;
    text.anchor = GODOT_TEXT_ANCHOR_END;
    text.color = ink;
    text.opacity = 1.0;
    count_text_.draw(canvas, frame, text);
    return true;
}

bool ItemWidget::reading(const ItemWidgetCarry& carry, bool stack, std::vector<HudRun>& out,
                         std::string& failed) const {
    // The mockup's wt: a stack weighs its count of ones; the volume stays that of one.
    const double mass = stack ? carry.mass_kg * static_cast<double>(carry.count) : carry.mass_kg;
    out = size_word_;
    // ${D.len}: the length as the record writes it (0.3 CM for the seeds), never rounded to a whole centimetre.
    if (!fill_words(out, {HudPortValue{"cm", hud_shortest(carry.length_cm)}, HudPortValue{"kg", kilograms(mass)}},
                    failed)) {
        return false;
    }
    if (carry.volume_l >= volume_from_) {
        std::vector<HudRun> volume = volume_word_;
        if (!fill_words(volume, {HudPortValue{"l", hud_decimals(carry.volume_l, static_cast<int32_t>(decimals_))}},
                        failed)) {
            return false;
        }
        out.insert(out.end(), volume.begin(), volume.end());
    }
    if (stack) {
        std::vector<HudRun> count = count_word_;
        if (!fill_words(count, {HudPortValue{"n", hud_whole(carry.count)}}, failed)) {
            return false;
        }
        out.insert(out.end(), count.begin(), count.end());
    }
    return true;
}

std::string ItemWidget::kilograms(double value) const {
    // The mockup's kgf: finer below hud_load_fine_below, else to hud_load_decimals.
    const double decimals = value < fine_below_ ? fine_decimals_ : decimals_;
    return hud_decimals(value, static_cast<int32_t>(decimals));
}

}  // namespace ase::adp::godot
