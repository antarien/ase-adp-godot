/**
 * ASE GODOT ADAPTER - HUD HAND MODE
 *
 * @file        godot_hud_hand_mode.cpp
 * @brief       What the hand says and draws in each mode: the pour arcs, the rain, the verbs, the readings
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_hand_mode.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0454 (flora.patch.v1), N0449 (sky.weather.v1), N0462, N0461, N0463 (flora.root.v1), N0457,
 *              N0464 (height and slope under the brush), carrier N0448: gaps of the bridge, never of this unit
 * @created     2026-11-10
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_hand_mode.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/string.hpp>

#include <initializer_list>

namespace ase::adp::godot {

namespace {

::godot::Vector2 at(double x, double y) {
    return ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
}

}  // namespace

bool HandMode::bind(const UiTheme& theme, std::string& failed) {
    theme_ = &theme;
    chips_.clear();
    verb_places_.clear();
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_hand_x", &hand_x_},
             {"hud_hand_y", &hand_y_},
             {"hud_hand_pour_radius", &pour_radius_},
             {"hud_hand_pour_arc_step", &pour_step_},
             {"hud_hand_pour_arc_from", &pour_from_},
             {"hud_hand_pour_arc_to", &pour_to_},
             {"hud_hand_pour_arc_width", &pour_width_},
             {"hud_hand_pour_arcs", &pour_arcs_},
             {"hud_hand_pour_text_dy", &pour_text_dy_},
             {"hud_hand_pour_text_size", &pour_text_size_},
             {"hud_hand_pour_fill_dy", &pour_fill_dy_},
             {"hud_hand_pour_fill_size", &pour_fill_size_},
             {"hud_hand_rain_glyph_x", &rain_glyph_x_},
             {"hud_hand_rain_glyph_y", &rain_glyph_y_},
             {"hud_hand_rain_glyph_size", &rain_glyph_size_},
             {"hud_hand_rain_text_x", &rain_text_x_},
             {"hud_hand_rain_text_y", &rain_text_y_},
             {"hud_hand_rain_text_size", &rain_text_size_},
             {"hud_hand_root_text_x", &root_x_},
             {"hud_hand_root_text_y", &root_y_},
             {"hud_hand_root_text_size", &root_size_},
             {"hud_hand_verb_distance", &verb_distance_},
             {"hud_hand_verb_radius", &verb_radius_},
             {"hud_hand_verb_two_angle", &verb_two_},
             {"hud_hand_verb_three_angle", &verb_three_},
             {"hud_hand_verb_label_dy", &verb_label_dy_},
             {"hud_hand_verb_label_size", &verb_label_size_},
             {"hud_focus_soil_dry_below", &dry_below_},
             {"hud_load_decimals", &decimals_},
             {"hud_load_fine_below", &fine_below_},
             {"hud_load_fine_decimals", &fine_decimals_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"DOMAIN_WATER_HELD", &held_},
                                                                       {"OVERLAY_INK_SOFT", &soft_},
                                                                       {"OVERLAY_INK_FAINT", &faint_},
                                                                       {"DOMAIN_SKY", &sky_},
                                                                       {"DOMAIN_ALERT", &alert_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{
             {"water_empty", &water_empty_},       {"water_release", &water_release_},
             {"water_hold", &water_hold_},         {"water_after", &water_after_},
             {"water_fill", &water_fill_},         {"row_soil", &row_soil_},
             {"words.dry", &dry_},                 {"words.moist", &moist_},
             {"rain_in", &rain_in_},               {"rain_in_one", &rain_in_one_},
             {"rain_coming", &rain_coming_},       {"pour_mm", &pour_mm_},
             {"prune_ask", &prune_ask_},           {"prune_left", &prune_left_},
             {"sow_none", &sow_none_},             {"sow_ask", &sow_ask_},
             {"sow_alive", &sow_alive_},           {"sow_left", &sow_left_},
             {"sap_no_vessel", &sap_no_vessel_},   {"sap_no_spout", &sap_no_spout_},
             {"sap_ask", &sap_ask_},               {"sap_left", &sap_left_},
             {"plug_ask", &plug_ask_},             {"plug_vessel", &plug_vessel_},
             {"use_hand", &use_hand_},             {"weight", &weight_},
             {"hand_number", &number_},            {"hand_weight", &kg_},
             {"hand_litres", &litres_},            {"roots", &roots_},
             {"verb_ask.bed_start", &bed_start_},
             {"verb_ask.bed_end", &bed_end_},      {"verb_reading.level", &read_level_},
             {"verb_reading.bed", &read_bed_},     {"verb_reading.measure", &read_measure_},
             {"verb_reading.radius", &read_radius_}, {"verb_reading.height", &read_height_},
             {"verb_reading.slope", &read_slope_}, {"verb_reading.plus", &read_plus_}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }
    if (!label_text_.bind_label(theme, failed) || !rain_text_.bind_label(theme, failed) ||
        !data_text_.bind_data(theme, failed) || !fill_text_.bind_data(theme, failed) || !ring_.bind(theme, failed) ||
        !bind_word_icon(theme, "rain_glyph", rain_icon_, failed)) {
        return false;
    }
    const std::vector<uint32_t>& levels = theme.spec().levels;
    if (levels.size() < 3) {
        failed = "hud.perception.levels";
        return false;
    }
    level_words_ = levels[0];
    level_values_ = levels[1];
    level_detail_ = levels[2];
    return true;
}

HandMode::VerbChip* HandMode::chip(const std::string& verb, std::string& failed) {
    for (VerbChip& entry : chips_) {
        if (entry.verb == verb) {
            return &entry;
        }
    }
    VerbChip made;
    made.verb = verb;
    const std::string stem = GODOT_HUD_VERB_TOKEN + verb;
    ::godot::String text;
    if (!theme_->token_text(stem + GODOT_HUD_VERB_TONE_TAIL, text)) {
        failed = stem + GODOT_HUD_VERB_TONE_TAIL;
        return nullptr;
    }
    const std::string code = text.utf8().get_data();
    if (!theme_->tone(code, made.tone)) {
        failed = "DOMAIN_" + code;
        return nullptr;
    }
    if (!theme_->token_text(stem + GODOT_HUD_VERB_OP_TAIL, text) || text.is_empty()) {
        failed = stem + GODOT_HUD_VERB_OP_TAIL;
        return nullptr;
    }
    made.op = text.utf8().get_data();
    std::string icon;
    if (!word_icon(*theme_, "verb_glyph." + verb, icon, failed) || !made.chip.bind(*theme_, failed) ||
        !made.chip.set_glyph(icon, std::string(), failed) || !word("verbs." + verb, made.name, failed) ||
        !word("verb_case." + verb, made.case_words, failed) || !made.name_text.bind_label(*theme_, failed)) {
        return nullptr;
    }
    chips_.push_back(made);
    return &chips_.back();
}

bool HandMode::word(const std::string& key, std::vector<HudRun>& out, std::string& failed) const {
    if (!theme_->words(key, out)) {
        failed = "texts.hud." + key;
        return false;
    }
    return true;
}

bool HandMode::fill(const std::vector<HudRun>& word, const std::vector<HudPortValue>& values,
                    std::vector<HudRun>& out, std::string& failed) const {
    out = word;
    return fill_words(out, values, failed);
}

bool HandMode::say(UiPen& canvas, const ::godot::Transform2D& frame, UiTextBlock& block,
                   const std::vector<HudRun>& runs, double x, double y, double size, double anchor,
                   const ::godot::Color& color, std::string& failed) {
    if (!block.set(runs, failed)) {
        return false;
    }
    UiTextPlace place;
    place.at = at(x, y);
    place.size = size;
    place.anchor = anchor;
    place.color = color;
    place.opacity = 1.0;
    block.draw(canvas, frame, place);
    canvas.draw_set_transform_matrix(frame);
    return true;
}

std::string HandMode::count(uint32_t value) {
    return hud_whole(static_cast<int64_t>(value));
}

bool HandMode::water(UiPen& canvas, const ::godot::Transform2D& frame, const HandSlotFrame& now,
                     const HandThing& thing, const ::godot::Color& tone, HandLines& lines, std::string& failed) {
    lines.hint = thing.fill_l <= 0.0 ? water_empty_ : now.holding ? water_release_ : water_hold_;
    lines.label = row_soil_;
    const ::godot::Vector2 hand = at(hand_x_, hand_y_);

    // One arc per litre: poured in the tone, still in the can held, else faint.
    const double poured = ::godot::Math::round(now.poured_l);
    std::vector<::godot::Color> arcs;
    for (double index = 0.0; index < pour_arcs_; index += 1.0) {
        arcs.push_back(index < poured ? tone : index < thing.fill_l ? held_ : faint_);
    }
    canvas.draw_set_transform_matrix(frame);
    ring_.steps(canvas, hand, pour_radius_, pour_from_, pour_to_, pour_step_, arcs, pour_width_, 1.0);

    std::vector<HudRun> runs;
    const std::string before = hud_decimals(now.soil_rel * GODOT_PERCENT, 0);
    const std::string after = hud_decimals(now.after_rel * GODOT_PERCENT, 0);
    if (now.level >= level_values_) {
        if (!fill(water_after_, {HudPortValue{"now", before}, HudPortValue{"after", after}}, lines.value, failed)) {
            return false;
        }
    } else {
        lines.value = now.after_rel < dry_below_ ? dry_ : moist_;
    }
    // The port values stand named for the whole block: a list built inside the || chain would die at its end
    // while fill() and say() still run on the words it filled.
    if (now.pour_mm > 0.0) {
        const std::vector<HudPortValue> pour_values{HudPortValue{"mm", hud_decimals(now.pour_mm, 0)}};
        if (!fill(pour_mm_, pour_values, runs, failed) ||
            !say(canvas, frame, data_text_, runs, hand_x_, hand_y_ + pour_text_dy_, pour_text_size_,
                 GODOT_TEXT_ANCHOR_MIDDLE, tone, failed)) {
            return false;
        }
    }
    if (now.level >= level_values_) {
        const std::vector<HudPortValue> can_values{HudPortValue{"fill", hud_decimals(thing.fill_l, 0)},
                                                   HudPortValue{"cap", hud_decimals(thing.cap_l, 0)}};
        if (!fill(water_fill_, can_values, runs, failed) ||
            !say(canvas, frame, fill_text_, runs, hand_x_, hand_y_ + pour_fill_dy_, pour_fill_size_,
                 GODOT_TEXT_ANCHOR_MIDDLE, faint_, failed)) {
            return false;
        }
    }

    // The rain the forecast brings: sky.weather.v1's, or the mockup's the bridge writes while the port is
    // silent (bridge_weather). A forecast without rain draws no warning, as the mockup's rainIn() returns none.
    if (!now.rain_coming) {
        return true;
    }
    rain_icon_.draw(canvas, frame, at(rain_glyph_x_, rain_glyph_y_), rain_glyph_size_, sky_, 1.0);
    canvas.draw_set_transform_matrix(frame);
    if (now.level >= level_detail_) {
        if (!fill(now.rain_phases == 1 ? rain_in_one_ : rain_in_,
                  {HudPortValue{"phases", count(now.rain_phases)},
                   HudPortValue{"mm", hud_decimals(now.rain_mm, static_cast<int32_t>(decimals_))}},
                  runs, failed)) {
            return false;
        }
    } else {
        runs = rain_coming_;
    }
    return say(canvas, frame, rain_text_, runs, rain_text_x_, rain_text_y_, rain_text_size_, GODOT_TEXT_ANCHOR_START,
               sky_, failed);
}

bool HandMode::terrain(UiPen& canvas, const ::godot::Transform2D& frame, const HandSlotFrame& now,
                       const HandThing& thing, HandLines& lines, std::string& failed) {
    VerbChip* chosen = nullptr;
    for (const std::string& verb : thing.verbs) {
        if (verb == now.verb) {
            chosen = chip(verb, failed);
            if (chosen == nullptr) {
                return false;
            }
        }
    }
    if (chosen == nullptr) {
        failed = "HandSlot: the mode terrain names no verb of " + thing.id + " (" + now.verb + ")";
        return false;
    }
    const std::string op = chosen->op;
    lines.case_words = chosen->case_words;
    if (op == GODOT_HUD_VERB_OP_BED) {
        lines.hint = now.bed_started ? bed_end_ : bed_start_;
        lines.label = read_bed_;
    } else {
        if (!word("verb_ask." + op, lines.hint, failed)) {
            return false;
        }
        if (op == GODOT_HUD_VERB_OP_LEVEL) {
            if (!fill(read_level_, {HudPortValue{"place", now.area}}, lines.label, failed)) {
                return false;
            }
        } else if (op == GODOT_HUD_VERB_OP_MEASURE) {
            lines.label = read_measure_;
        } else if (!fill(read_radius_,
                         {HudPortValue{"m", hud_decimals(now.brush_m, static_cast<int32_t>(decimals_))}}, lines.label,
                         failed)) {
            return false;
        }
    }
    if (now.brush_on && now.level >= level_values_) {
        std::string sign;
        if (now.height_m >= 0.0 && !word_text(read_plus_, sign, failed)) {
            return false;
        }
        if (!fill(read_height_,
                  {HudPortValue{"sign", sign},
                   HudPortValue{"m", hud_decimals(now.height_m, static_cast<int32_t>(decimals_))}},
                  lines.value, failed)) {
            return false;
        }
        std::vector<HudRun> slope;
        if (now.level >= level_detail_) {
            if (!fill(read_slope_, {HudPortValue{"deg", hud_decimals(now.slope_deg, 0)}}, slope, failed)) {
                return false;
            }
            lines.value.insert(lines.value.end(), slope.begin(), slope.end());
        }
    }

    // The root zone under the brush.
    if (now.brush_on && now.in_roots && op != GODOT_HUD_VERB_OP_MEASURE && op != GODOT_HUD_VERB_OP_LEVEL &&
        !say(canvas, frame, label_text_, roots_, root_x_, root_y_, root_size_, GODOT_TEXT_ANCHOR_END, alert_, failed)) {
        return false;
    }

    // The verbs on their arc above the hand.
    const std::size_t verbs = thing.verbs.size();
    for (std::size_t index = 0; index < verbs; ++index) {
        VerbChip* entry = chip(thing.verbs[index], failed);
        if (entry == nullptr) {
            return false;
        }
        const double angle = verbs == 1 ? 0.0
                             : verbs == 2 ? (index == 0 ? -verb_two_ : verb_two_)
                                          : -verb_three_ + static_cast<double>(index) * verb_three_;
        UiHexChipPlace place;
        place.centre = UiRing::point(at(hand_x_, hand_y_), verb_distance_, angle);
        place.radius = verb_radius_;
        place.tone = entry->tone;
        place.on = entry->verb == now.verb;
        place.elapsed = now.elapsed;
        place.still = now.still;
        entry->chip.draw(canvas, frame, place);
        canvas.draw_set_transform_matrix(frame);
        verb_places_.emplace_back(entry->verb, place);
        if ((verbs > 1 || now.level == level_words_) &&
            !say(canvas, frame, entry->name_text, entry->name, place.centre.x, place.centre.y + verb_label_dy_,
                 verb_label_size_, GODOT_TEXT_ANCHOR_MIDDLE, place.on ? entry->tone : soft_, failed)) {
            return false;
        }
    }
    return true;
}

void HandMode::warm(const UiPen& canvas, const ::godot::Transform2D& frame, int64_t& budget) {
    label_text_.warm(canvas, frame, root_size_, budget);
    rain_text_.warm(canvas, frame, rain_text_size_, budget);
    data_text_.warm(canvas, frame, pour_text_size_, budget);
    fill_text_.warm(canvas, frame, pour_fill_size_, budget);
    for (VerbChip& entry : chips_) {
        entry.name_text.warm(canvas, frame, verb_label_size_, budget);
    }
}

bool HandMode::draw(UiPen& canvas, const ::godot::Transform2D& frame, const HandSlotFrame& now,
                    const HandThing& thing, const HandItemLook& look, HandLines& lines, std::string& failed) {
    verb_places_.clear();
    lines = HandLines();
    lines.case_words = look.case_words;
    canvas.draw_set_transform_matrix(frame);
    bool drawn = true;
    if (now.mode == GODOT_HUD_MODE_WATER) {
        drawn = water(canvas, frame, now, thing, look.tone, lines, failed);
    } else if (now.mode == GODOT_HUD_MODE_PRUNE) {
        lines.hint = prune_ask_;
        lines.label = prune_left_;
        drawn = fill(number_, {HudPortValue{"n", count(now.prune_left)}}, lines.value, failed);
    } else if (now.mode == GODOT_HUD_MODE_SOW) {
        lines.label = sow_left_;
        lines.hint = now.seeds == 0 ? sow_none_ : now.dead ? sow_ask_ : sow_alive_;
        drawn = fill(number_, {HudPortValue{"n", count(now.seeds)}}, lines.value, failed);
    } else if (now.mode == GODOT_HUD_MODE_TERRAIN) {
        drawn = terrain(canvas, frame, now, thing, lines, failed);
    } else if (now.mode == GODOT_HUD_MODE_SAP) {
        lines.hint = !now.has_vessel ? sap_no_vessel_ : now.spouts == 0 ? sap_no_spout_ : sap_ask_;
        lines.label = sap_left_;
        drawn = fill(number_, {HudPortValue{"n", count(now.spouts)}}, lines.value, failed);
    } else if (now.mode == GODOT_HUD_MODE_PLUG) {
        lines.hint = plug_ask_;
        lines.label = plug_vessel_;
        drawn = fill(litres_, {HudPortValue{"l", hud_decimals(now.tap_l, static_cast<int32_t>(decimals_))}},
                     lines.value, failed);
    } else {
        lines.hint = look.hold.empty() ? use_hand_ : look.hold;
        lines.label = weight_;
        const int32_t places = static_cast<int32_t>(thing.kg < fine_below_ ? fine_decimals_ : decimals_);
        drawn = fill(kg_, {HudPortValue{"kg", hud_decimals(thing.kg, places)}}, lines.value, failed);
    }
    canvas.draw_set_transform_matrix(frame);
    return drawn;
}

std::string HandMode::hit(const ::godot::Vector2& point, std::string& key) const {
    key.clear();
    for (const std::pair<std::string, UiHexChipPlace>& entry : verb_places_) {
        if (hud_hex_contains(entry.second.centre, entry.second.radius, point)) {
            key = entry.first;
            return GODOT_HUD_ACT_VERB;
        }
    }
    return std::string();
}

}  // namespace ase::adp::godot
