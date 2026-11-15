/**
 * ASE GODOT ADAPTER - HUD DAY RING
 *
 * @file        godot_hud_day_ring.cpp
 * @brief       The day ring of the calendar sheet: the light of the day, the six phases, the sun or moon, the date
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_day_ring.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @register    N0460 (flora.day.v1: shares of the day, sunrise, sunset; flora.phase.v1: phase shares), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_day_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/types.hpp>

#include <godot_cpp/variant/string.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

namespace {

/** The angle of an hour the module placed at `share` of the day. */
double share_angle(double share) {
    return share * GODOT_RING_TURN_DEG;
}

}  // namespace

bool DayRing::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_year_x", &x_},
             {"hud_year_y", &y_},
             {"hud_day_radius", &radius_},
             {"hud_day_light_width", &light_width_},
             {"hud_day_light_alpha", &light_alpha_},
             {"hud_day_light_delay", &light_delay_},
             {"hud_day_tick_half", &tick_half_},
             {"hud_day_phase_inset", &phase_inset_},
             {"hud_day_phase_dy", &phase_dy_},
             {"hud_day_phase_size", &phase_size_},
             {"hud_day_sun_gap", &sun_gap_},
             {"hud_day_sun_size", &sun_size_},
             {"hud_day_tum_dy", &tum_dy_},
             {"hud_day_tum_size", &tum_size_},
             {"hud_day_lun_dy", &lun_dy_},
             {"hud_day_lun_size", &lun_size_},
             {"hud_day_hour_dy", &hour_dy_},
             {"hud_day_hour_size", &hour_size_},
             {"hud_day_hour_decimals", &hour_decimals_},
             {"hud_line_hair", &line_hair_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_INK_SOFT", &soft_},
                                                                       {"OVERLAY_INK_FAINT", &faint_},
                                                                       {"DOMAIN_TIME", &time_},
                                                                       {"DOMAIN_CUT", &cut_},
                                                                       {"DOMAIN_SKY", &sky_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{{"day_of", &day_of_},
                                                                            {"date_of", &date_of_},
                                                                            {"hour_of", &hour_of_}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }

    // The phases in the order of GODOT_PERIOD_FIRST .. GODOT_PERIOD_LAST, one word each.
    if (!bind_phase_words(theme, phase_words_, failed)) {
        return false;
    }

    return ring_.bind(theme, failed) && sun_.bind(theme, "ICON_SUN", failed) &&
           moon_.bind(theme, "ICON_MOON", failed) && phase_.bind_label(theme, failed) &&
           tum_.bind_title(theme, failed) && lun_.bind_data(theme, failed) && hour_.bind_data(theme, failed);
}

std::string DayRing::decimal(double value) const {
    const int decimals = static_cast<int>(hour_decimals_);
    return hud_decimals(value, decimals);
}

bool DayRing::draw_day(UiPen& canvas, const ::godot::Transform2D& frame, const DayRingFrame& now,
                       std::string& failed) {
    if (now.phase_shares.size() != phase_words_.size()) {
        failed = "DayRing: the clock record must give one start per period of GODOT_PERIOD_*";
        return false;
    }
    const ::godot::Vector2 centre(static_cast<float>(x_), static_cast<float>(y_));

    // The hours of light, from the sunrise to the sunset.
    const double rise = share_angle(now.sunrise_share);
    double set = share_angle(now.sunset_share);
    if (set <= rise) {
        set += GODOT_RING_TURN_DEG;
    }
    UiArcStyle style;
    style.opacity = light_alpha_;
    style.drawn = ring_.drawn(now.elapsed, light_delay_, now.still);
    ring_.arc(canvas, centre, radius_, rise, set, cut_, light_width_, style);

    // One tick at the start of each phase, its word at its middle.
    for (size_t index = 0; index < phase_words_.size(); ++index) {
        const double from = share_angle(now.phase_shares[index]);
        double to = index + 1 < phase_words_.size() ? share_angle(now.phase_shares[index + 1])
                                                    : share_angle(now.phase_shares.front()) + GODOT_RING_TURN_DEG;
        if (to <= from) {
            to += GODOT_RING_TURN_DEG;
        }
        canvas.draw_line(UiRing::point(centre, radius_ - tick_half_, from),
                         UiRing::point(centre, radius_ + tick_half_, from), soft_, static_cast<float>(line_hair_),
                         true);
        if (!phase_.set(phase_words_[index], failed)) {
            return false;
        }
        UiTextPlace word;
        word.at = UiRing::point(centre, radius_ - phase_inset_, (from + to) * GODOT_HALF) +
                  ::godot::Vector2(0.0f, static_cast<float>(phase_dy_));
        word.size = phase_size_;
        word.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
        word.color = static_cast<int32_t>(index) == now.period - GODOT_PERIOD_FIRST ? time_ : soft_;
        word.opacity = 1.0;
        word.elapsed = now.elapsed;
        word.still = now.still;
        phase_.draw(canvas, frame, word);
    }

    // The sun or, under the horizon, the moon at the day's place.
    const ::godot::Vector2 body = UiRing::point(centre, radius_ + sun_gap_, share_angle(now.day_share));
    if (now.night) {
        moon_.draw(canvas, frame, body, sun_size_, sky_, 1.0);
    } else {
        sun_.draw(canvas, frame, body, sun_size_, cut_, 1.0);
    }
    return true;
}

bool DayRing::draw_date(UiPen& canvas, const ::godot::Transform2D& frame, const DayRingFrame& now,
                        std::string& failed) {
    const ::godot::Vector2 centre(static_cast<float>(x_), static_cast<float>(y_));
    std::string phase;
    if (!word_text(phase_words_[static_cast<size_t>(now.period - GODOT_PERIOD_FIRST)], phase, failed)) {
        return false;
    }
    std::vector<HudRun> tum = day_of_;
    std::vector<HudRun> lun = date_of_;
    std::vector<HudRun> hour = hour_of_;
    if (!fill_words(tum, {HudPortValue{"tum", hud_whole(now.tum)}}, failed) ||
        !fill_words(lun, {HudPortValue{"lun", hud_whole(now.lun)}, HudPortValue{"vec", hud_whole(now.vec)}}, failed) ||
        !fill_words(hour, {HudPortValue{"hour", decimal(now.hour)}, HudPortValue{"phase", phase}}, failed) ||
        !tum_.set(tum, failed) || !lun_.set(lun, failed) || !hour_.set(hour, failed)) {
        return false;
    }
    // Three centred lines under each other: the TUM in the ink of class tb, LUN with VEC, the hour with its phase.
    struct Line {
        UiTextBlock* block;
        double dy;
        double size;
        ::godot::Color color;
    };
    for (const Line& line : {Line{&tum_, tum_dy_, tum_size_, tum_.ink()}, Line{&lun_, lun_dy_, lun_size_, soft_},
                             Line{&hour_, hour_dy_, hour_size_, faint_}}) {
        UiTextPlace place;
        place.at = centre + ::godot::Vector2(0.0f, static_cast<float>(line.dy));
        place.size = line.size;
        place.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
        place.color = line.color;
        place.opacity = 1.0;
        place.elapsed = now.elapsed;
        place.still = now.still;
        line.block->draw(canvas, frame, place);
    }
    return true;
}

bool DayRing::draw(UiPen& canvas, const ::godot::Transform2D& frame, const DayRingFrame& now,
                   std::string& failed) {
    if (now.period < GODOT_PERIOD_FIRST || now.period > GODOT_PERIOD_LAST) {
        failed = "DayRing: the clock record's phase " + hud_whole(now.period) + " lies outside GODOT_PERIOD_*";
        return false;
    }
    canvas.draw_set_transform_matrix(frame);
    ring_.circle(canvas, ::godot::Vector2(static_cast<float>(x_), static_cast<float>(y_)), radius_, faint_,
                 line_hair_, 1.0);
    const bool drawn = draw_day(canvas, frame, now, failed) && draw_date(canvas, frame, now, failed);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return drawn;
}

}  // namespace ase::adp::godot
