/**
 * ASE GODOT ADAPTER - HUD SIGN READING
 *
 * @file        godot_hud_sign_reading.cpp
 * @brief       The reading of the open sign: leader, title, sentence, bar, numbers, case and the offer of its tool
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block SignMarker, see godot_hud_sign_reading.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @category    ecs/module
 * @usecase     usecase:100370 - wilt from drought
 * @actor       Hänge-Birke (Betula pendula)
 * @register    N0461, N0451, N0457, N0460, carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_sign_reading.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool SignReading::bind_sign(const UiTheme& theme, const HudSignSpec& spec, Sign& out, std::string& failed) const {
    const std::string stem = "hud_sign_" + spec.name;
    ::godot::String text;
    if (!theme.token_text(stem + "_tone", text)) {
        failed = stem + "_tone";
        return false;
    }
    const std::string code = text.utf8().get_data();
    if (!theme.tone(code, out.tone)) {
        failed = "DOMAIN_" + code;
        return false;
    }
    if (!theme.token_text(stem + "_bar", text) || text.is_empty()) {
        failed = stem + "_bar";
        return false;
    }
    out.bar = text.utf8().get_data();
    if (out.bar == GODOT_HUD_SIGN_BAR_NONE) {
        out.bar.clear();
    }
    const std::string key = "signs." + spec.name;
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{
             {".title", &out.title}, {".line", &out.line}, {".reading", &out.reading}, {".case", &out.case_words}}) {
        if (!theme.words(key + slot.first, *slot.second)) {
            failed = "texts.hud." + key + slot.first;
            return false;
        }
    }
    // A sign that alerts on a mark names what holds while that mark holds (the mockup's alert form).
    if (!spec.alert_when.empty()) {
        if (!theme.words(key + ".title_full", out.title_full)) {
            failed = "texts.hud." + key + ".title_full";
            return false;
        }
        if (!theme.words(key + ".line_full", out.line_full)) {
            failed = "texts.hud." + key + ".line_full";
            return false;
        }
    }
    return true;
}

bool SignReading::bind_offer(const UiTheme& theme, const HudToolSpec& spec, Offer& out, std::string& failed) const {
    const std::string token = "hud_fan_" + spec.name + "_tone";
    ::godot::String text;
    if (!theme.token_text(token, text)) {
        failed = token;
        return false;
    }
    const std::string code = text.utf8().get_data();
    if (!theme.tone(code, out.tone)) {
        failed = "DOMAIN_" + code;
        return false;
    }
    out.thing = !spec.has.empty();
    out.has = spec.has;
    if (out.thing) {
        return word_line(theme, "items." + spec.has + ".name", out.item, failed) &&
               out.drawing.bind(theme, failed) && out.drawing.set(spec.has, failed);
    }
    // A tool without an item names the word its SHA token gives, and shows its glyph.
    const std::string offer = "hud_fan_" + spec.name + "_offer";
    if (!theme.token_text(offer, text)) {
        failed = offer;
        return false;
    }
    const std::string word = text.utf8().get_data();
    if (!theme.words(word, out.word)) {
        failed = "texts.hud." + word;
        return false;
    }
    return word_icon(theme, "fan_glyph." + spec.name, out.icon, failed);
}

bool SignReading::bind(const UiTheme& theme, std::string& failed) {
    theme_ = &theme;
    offer_place_ = UiHexChipPlace{};
    offer_tool_ = UINT32_MAX;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_sign_split_x", &split_x_},
             {"hud_sign_anchor_gap", &anchor_gap_},
             {"hud_sign_elbow_dx", &elbow_dx_},
             {"hud_sign_elbow_dy", &elbow_dy_},
             {"hud_sign_column_gap", &column_gap_},
             {"hud_sign_column_right_max", &column_right_max_},
             {"hud_sign_column_left_min", &column_left_min_},
             {"hud_sign_run", &run_},
             {"hud_sign_title_dy", &title_dy_},
             {"hud_sign_title_size", &title_size_},
             {"hud_sign_title_delay", &title_delay_},
             {"hud_sign_sentence_dy", &sentence_dy_},
             {"hud_sign_sentence_size", &sentence_size_},
             {"hud_sign_sentence_delay", &sentence_delay_},
             {"hud_sign_line_step", &line_step_},
             {"hud_sign_bar_w", &bar_w_},
             {"hud_sign_bar_h", &bar_h_},
             {"hud_sign_bar_dy", &bar_dy_},
             {"hud_sign_bar_delay", &bar_delay_},
             {"hud_sign_bar_step", &bar_step_},
             {"hud_sign_numbers_size", &numbers_size_},
             {"hud_sign_numbers_delay", &numbers_delay_},
             {"hud_sign_numbers_step", &numbers_step_},
             {"hud_sign_case_size", &case_size_},
             {"hud_sign_case_delay", &case_delay_},
             {"hud_sign_case_step", &case_step_},
             {"hud_sign_offer_dx", &offer_dx_},
             {"hud_sign_offer_dy", &offer_dy_},
             {"hud_sign_offer_thing_radius", &offer_thing_radius_},
             {"hud_sign_offer_glyph_radius", &offer_glyph_radius_},
             {"hud_sign_offer_delay", &offer_delay_},
             {"hud_sign_offer_label_dx", &offer_label_dx_},
             {"hud_sign_offer_label_dy", &offer_label_dy_},
             {"hud_sign_offer_label_size", &offer_label_size_},
             {"hud_sign_offer_label_delay", &offer_label_delay_},
             {"hud_sign_offer_where_dy", &offer_where_dy_},
             {"hud_sign_offer_where_size", &offer_where_size_},
             {"hud_sign_offer_where_delay", &offer_where_delay_},
             {"hud_line", &line_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_INK", &ink_},
                                                                         {"OVERLAY_INK_FAINT", &faint_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{
             {"take", &take_words_}, {"missing", &missing_words_}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }

    const HudContextSpec& spec = theme.spec();
    signs_.clear();
    for (const HudSignSpec& sign : spec.signs) {
        Sign made;
        if (!bind_sign(theme, sign, made, failed)) {
            return false;
        }
        signs_.push_back(made);
    }
    offers_.clear();
    for (const HudToolSpec& tool : spec.tools) {
        Offer made;
        if (!bind_offer(theme, tool, made, failed)) {
            return false;
        }
        offers_.push_back(made);
    }
    if (spec.levels.size() < 3u) {
        failed = "hud.perception.levels";
        return false;
    }
    level_bars_ = spec.levels[1];
    level_numbers_ = spec.levels[2];

    return fade_.bind(theme, "hud_anim_fade", failed) && leader_.bind(theme, failed) && chip_.bind(theme, failed) &&
           title_.bind_title(theme, failed) && sentence_.bind_label(theme, failed) &&
           reading_.bind_data(theme, failed) && case_.bind_data(theme, failed) &&
           offer_label_.bind_label(theme, failed) && where_.bind_data(theme, failed);
}

void SignReading::draw_line(UiPen& canvas, const ::godot::Transform2D& frame,
                            const SignReadingFrame& now, UiTextBlock& block, const Column& column, double y,
                            double size, const ::godot::Color& color, double delay) const {
    UiTextPlace place;
    place.at = ::godot::Vector2(static_cast<float>(column.bx), static_cast<float>(y));
    place.size = size;
    place.anchor = column.anchor;
    place.color = color;
    place.opacity = 1.0;
    place.fade = true;
    place.delay = delay;
    place.elapsed = now.opened;
    place.still = now.still;
    block.draw(canvas, frame, place);
}

bool SignReading::draw_bar(UiPen& canvas, const ::godot::Transform2D& frame,
                           const SignReadingFrame& now, const Sign& sign, const Column& column, double yy,
                           std::string& failed) const {
    for (const SignValue& value : *now.values) {
        if (value.name != sign.bar) {
            continue;
        }
        const double shown = fade_.progress(now.opened, bar_delay_, now.still);
        const double x0 = column.dir > 0.0 ? column.bx : column.bx - bar_w_;
        const double y0 = yy + bar_dy_;
        ::godot::Color track = faint_;
        track.a *= static_cast<float>(shown);
        ::godot::Color fill = sign.tone;
        fill.a *= static_cast<float>(shown);
        canvas.draw_set_transform_matrix(frame);
        canvas.draw_rect(::godot::Rect2(static_cast<float>(x0), static_cast<float>(y0), static_cast<float>(bar_w_),
                                        static_cast<float>(bar_h_)),
                         track, true);
        canvas.draw_rect(::godot::Rect2(static_cast<float>(x0), static_cast<float>(y0),
                                        static_cast<float>(bar_w_ * value.share), static_cast<float>(bar_h_)),
                         fill, true);
        canvas.draw_set_transform_matrix(::godot::Transform2D());
        return true;
    }
    failed = "SignReading: no port value " + sign.bar + " for the bar (hud_sign_<id>_bar)";
    return false;
}

bool SignReading::draw_numbers(UiPen& canvas, const ::godot::Transform2D& frame,
                               const SignReadingFrame& now, const Sign& sign, const Column& column, double yy,
                               std::string& failed) {
    // Every placeholder of the reading from the frame's values (the mockup's r3 template over S).
    std::vector<HudPortValue> filled;
    for (const SignValue& value : *now.values) {
        filled.push_back(HudPortValue{value.name, value.text});
    }
    std::vector<HudRun> runs = sign.reading;
    if (!fill_words(runs, filled, failed) || !reading_.set(runs, failed)) {
        return false;
    }
    draw_line(canvas, frame, now, reading_, column, yy, numbers_size_, ink_, numbers_delay_);
    return true;
}

bool SignReading::draw_offer(UiPen& canvas, const ::godot::Transform2D& frame,
                             const SignReadingFrame& now, const Column& column, double yy, std::string& failed) {
    const uint32_t tool = now.shown.offer;
    if (tool >= offers_.size()) {
        failed = "SignReading: offer " + std::to_string(tool) + " outside hud.tools";
        return false;
    }
    const Offer& offer = offers_[tool];
    if (!(offer.thing ? chip_.set_thing(std::string(), failed)
                      : chip_.set_glyph(offer.icon, std::string(), failed))) {
        return false;
    }
    // HXI(hx, hy, 14, item, tone) or HX(hx, hy, 13, glyph, tone), fading in after hud_sign_offer_delay.
    UiHexChipPlace place;
    place.centre = ::godot::Vector2(static_cast<float>(column.bx + column.dir * offer_dx_),
                                    static_cast<float>(yy + offer_dy_));
    place.radius = offer.thing ? offer_thing_radius_ : offer_glyph_radius_;
    place.tone = offer.tone;
    place.fade = true;
    place.delay = offer_delay_;
    place.elapsed = now.opened;
    place.still = now.still;
    chip_.draw(canvas, frame, place);
    if (offer.thing) {
        const UiHexChipPicture picture = chip_.picture(place);
        UiItemSpritePlace thing;
        thing.centre = picture.centre;
        thing.box = picture.box;
        thing.opacity = picture.opacity;
        offer.drawing.draw(canvas, frame, thing);
    }
    offer_place_ = place;
    offer_tool_ = tool;

    // Beside it: TAKE {item}, {item} MISSING, or the word of a tool without an item.
    std::vector<HudRun> runs = offer.word;
    if (offer.thing) {
        runs = now.shown.offer_got ? take_words_ : missing_words_;
        if (!fill_words(runs, {HudPortValue{"item", offer.item}}, failed)) {
            return false;
        }
    }
    if (!offer_label_.set(runs, failed)) {
        return false;
    }
    const Column label{place.centre.x + column.dir * offer_label_dx_, column.dir, column.anchor};
    const double hy = place.centre.y;
    draw_line(canvas, frame, now, offer_label_, label, hy + offer_label_dy_, offer_label_size_,
              now.shown.offer_got ? offer.tone : faint_, offer_label_delay_);

    // From levels[2] on, where the item in possession lies: where(findIt(item)), the frame's place of its first
    // carried piece; an item no place is named for shows no line, as where() of no thing is empty.
    if (now.level < level_numbers_ || !offer.thing || !now.shown.offer_got || now.places == nullptr) {
        return true;
    }
    const char* word = nullptr;
    for (const SignPlace& place_of : *now.places) {
        if (place_of.item == offer.has) {
            word = place_of.where;
            break;
        }
    }
    if (word == nullptr) {
        return true;
    }
    std::vector<HudRun> where;
    if (!theme_->words(word, where)) {
        failed = std::string("texts.hud.") + word;
        return false;
    }
    if (!where_.set(where, failed)) {
        return false;
    }
    draw_line(canvas, frame, now, where_, label, hy + offer_where_dy_, offer_where_size_, faint_,
              offer_where_delay_);
    return true;
}

bool SignReading::draw(UiPen& canvas, const ::godot::Transform2D& frame, const SignReadingFrame& now,
                       std::string& failed) {
    offer_place_ = UiHexChipPlace{};
    offer_tool_ = UINT32_MAX;
    if (now.shown.sign >= signs_.size()) {
        failed = "SignReading: sign " + std::to_string(now.shown.sign) + " outside hud.signs";
        return false;
    }
    if (now.values == nullptr) {
        failed = "SignReading: no port values handed over";
        return false;
    }
    const Sign& sign = signs_[now.shown.sign];
    const double x = now.centre.x;
    const double y = now.centre.y;
    const bool right = x < split_x_;
    const double dir = right ? 1.0 : -1.0;
    const double ex = x + dir * elbow_dx_;
    const double ey = y + elbow_dy_;
    const double lx = ex + dir * column_gap_;
    const double bx = right ? (lx < column_right_max_ ? lx : column_right_max_)
                            : (lx > column_left_min_ ? lx : column_left_min_);
    const Column column{bx, dir, right ? GODOT_TEXT_ANCHOR_START : GODOT_TEXT_ANCHOR_END};

    // L([[x + dir * 6, y - 6], [ex, ey], [xe, ey]], tone), drawn with hud_anim_draw from the opening on.
    ::godot::PackedVector2Array leader;
    leader.push_back(::godot::Vector2(static_cast<float>(x + dir * anchor_gap_), static_cast<float>(y - anchor_gap_)));
    leader.push_back(::godot::Vector2(static_cast<float>(ex), static_cast<float>(ey)));
    leader.push_back(::godot::Vector2(static_cast<float>(bx + dir * run_), static_cast<float>(ey)));
    canvas.draw_set_transform_matrix(frame);
    leader_.draw(canvas, leader, sign.tone, line_, leader_.drawn(now.opened, 0.0, now.still));
    canvas.draw_set_transform_matrix(::godot::Transform2D());

    const bool full = now.shown.alert && !sign.title_full.empty();
    if (!title_.set(full ? sign.title_full : sign.title, failed)) {
        return false;
    }
    draw_line(canvas, frame, now, title_, column, ey + title_dy_, title_size_, sign.tone, title_delay_);
    double yy = ey + sentence_dy_;
    if (!sentence_.set(full ? sign.line_full : sign.line, failed)) {
        return false;
    }
    draw_line(canvas, frame, now, sentence_, column, yy, sentence_size_, ink_, sentence_delay_);
    yy += line_step_;

    if (now.level >= level_bars_ && !sign.bar.empty()) {
        if (!draw_bar(canvas, frame, now, sign, column, yy, failed)) {
            return false;
        }
        yy += bar_step_;
    }
    if (now.level >= level_numbers_) {
        if (!draw_numbers(canvas, frame, now, sign, column, yy, failed)) {
            return false;
        }
        yy += numbers_step_;
        if (!case_.set(sign.case_words, failed)) {
            return false;
        }
        draw_line(canvas, frame, now, case_, column, yy, case_size_, faint_, case_delay_);
        yy += case_step_;
    }
    if (now.shown.offer == UINT32_MAX) {
        return true;
    }
    return draw_offer(canvas, frame, now, column, yy, failed);
}

bool SignReading::hit(const ::godot::Vector2& point, uint32_t& tool) const {
    if (offer_tool_ == UINT32_MAX || offer_place_.radius <= 0.0 || !chip_.hit(offer_place_, point)) {
        return false;
    }
    tool = offer_tool_;
    return true;
}

}  // namespace ase::adp::godot
