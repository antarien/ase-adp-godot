/**
 * ASE GODOT ADAPTER - HUD YEAR RING
 *
 * @file        godot_hud_year_ring.cpp
 * @brief       The year ring of the calendar sheet: the LUN of the year, the seasons, today, past events, the next stage
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_year_ring.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @register    N0460 (flora.season.v1: seasons, light hours; flora.day.v1: share of the year), N0450 (flora.trail.v1), N0454 (flora.patch.v1: next stage), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_year_ring.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

namespace {

/** The angle of a day the module placed at `share` of the year. */
double share_angle(double share) {
    return share * GODOT_RING_TURN_DEG;
}

}  // namespace

bool YearRing::bind(const UiTheme& theme, std::string& failed) {
    theme_ = &theme;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_year_x", &x_},
             {"hud_year_y", &y_},
             {"hud_year_radius", &radius_},
             {"hud_year_season_gap", &season_gap_},
             {"hud_year_season_trim", &season_trim_},
             {"hud_year_season_width", &season_width_},
             {"hud_year_season_stagger", &season_stagger_},
             {"hud_year_season_label_gap", &season_label_gap_},
             {"hud_year_season_label_size", &season_label_size_},
             {"hud_year_season_label_delay", &season_label_delay_},
             {"hud_year_light_dy", &light_dy_},
             {"hud_year_light_size", &light_size_},
             {"hud_year_light_delay", &light_delay_},
             {"hud_year_marks", &marks_},
             {"hud_year_tick_inner", &tick_inner_},
             {"hud_year_tick_outer", &tick_outer_},
             {"hud_year_number_inset", &number_inset_},
             {"hud_year_number_dy", &number_dy_},
             {"hud_year_number_size", &number_size_},
             {"hud_year_lun_inset", &lun_inset_},
             {"hud_year_lun_trim", &lun_trim_},
             {"hud_year_lun_width", &lun_width_},
             {"hud_year_lun_alpha", &lun_alpha_},
             {"hud_year_today_inner", &today_inner_},
             {"hud_year_today_outer", &today_outer_},
             {"hud_year_today_line", &today_line_},
             {"hud_year_event_inset", &event_inset_},
             {"hud_year_event_radius", &event_radius_},
             {"hud_year_next_gap", &next_gap_},
             {"hud_year_next_radius", &next_radius_},
             {"hud_year_next_line", &next_line_},
             {"hud_year_next_label_dx", &next_label_dx_},
             {"hud_year_next_label_dy", &next_label_dy_},
             {"hud_year_next_label_size", &next_label_size_},
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
                                                                       {"DOMAIN_LIFE", &life_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, std::vector<HudRun>*>& slot :
         std::initializer_list<std::pair<const char*, std::vector<HudRun>*>>{{"light", &light_words_}}) {
        if (!theme.words(slot.first, *slot.second)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
    }

    // The seasons in the order of the year, each word with its own colour.
    seasons_.clear();
    for (const std::pair<const char*, const char*>& slot : std::initializer_list<std::pair<const char*, const char*>>{
             {"seasons.0", "SEASON_GLACIX"},
             {"seasons.1", "SEASON_SALTATRIX"},
             {"seasons.2", "SEASON_IGNIX"},
             {"seasons.3", "SEASON_HALITRIX"}}) {
        YearRingSeason season;
        if (!theme.words(slot.first, season.word)) {
            failed = std::string("texts.hud.") + slot.first;
            return false;
        }
        if (!theme.color(slot.second, season.color)) {
            failed = slot.second;
            return false;
        }
        seasons_.push_back(season);
    }

    // The stages of the plant in their order, one word each.
    stage_words_.clear();
    for (const char* key : {"stages.0", "stages.1", "stages.2", "stages.3"}) {
        std::vector<HudRun> word;
        if (!theme.words(key, word)) {
            failed = std::string("texts.hud.") + key;
            return false;
        }
        stage_words_.push_back(word);
    }

    // The threshold of perception is the spec's level, never a number of this unit.
    const std::vector<uint32_t>& levels = theme.spec().levels;
    if (levels.size() < 2) {
        failed = "hud.perception.levels";
        return false;
    }
    level_light_ = levels[1];

    return ring_.bind(theme, failed) && season_.bind_label(theme, failed) && light_.bind_data(theme, failed) &&
           number_.bind_data(theme, failed) && stage_.bind_label(theme, failed);
}

double YearRing::month_angle(double lun, double lun_share) const {
    // B0280: ase-calendar places LUN m at (m - 1) whole LUN of the year; the last LUN is shorter, so the turn is never
    // cut into equal LUN parts - today's share of the year and the LUN marks stand on the same scale
    return (lun - 1.0) * lun_share * GODOT_RING_TURN_DEG;
}

uint32_t YearRing::mark_lun(uint32_t mark, uint32_t marks, uint32_t months) const {
    // LUN m begins at (m - 1) / months of the turn and mark k stands at k / marks of it; the first LUN at or
    // after the mark is 1 + ceil(k * months / marks), in whole numbers.
    return 1U + (mark * months + marks - 1U) / marks;
}

bool YearRing::draw_seasons(UiPen& canvas, const ::godot::Transform2D& frame,
                            const YearRingFrame& now, std::string& failed) {
    if (now.season_starts.size() != seasons_.size() || now.season_light.size() != seasons_.size()) {
        failed = "YearRing: the clock record must give one first LUN and one light per season of texts.hud.seasons";
        return false;
    }
    const ::godot::Vector2 centre(static_cast<float>(x_), static_cast<float>(y_));
    for (size_t index = 0; index < seasons_.size(); ++index) {
        const YearRingSeason& season = seasons_[index];
        const double from = month_angle(now.season_starts[index], now.lun_share);
        double to = month_angle(now.season_starts[(index + 1) % seasons_.size()], now.lun_share);
        if (to <= from) {
            to += GODOT_RING_TURN_DEG;
        }
        UiArcStyle style;
        style.opacity = 1.0;
        style.drawn = ring_.drawn(now.elapsed, season_stagger_ * static_cast<double>(index), now.still);
        ring_.arc(canvas, centre, radius_ + season_gap_, from + season_trim_, to - season_trim_, season.color,
                  season_width_, style);

        const ::godot::Vector2 label = UiRing::point(centre, radius_ + season_label_gap_, (from + to) / 2.0);
        if (!season_.set(season.word, failed)) {
            return false;
        }
        UiTextPlace word;
        word.at = label;
        word.size = season_label_size_;
        word.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
        word.color = season.color;
        word.opacity = 1.0;
        word.fade = true;
        word.delay = season_label_delay_;
        word.elapsed = now.elapsed;
        word.still = now.still;
        season_.draw(canvas, frame, word);

        if (now.level >= level_light_) {
            std::vector<HudRun> runs = light_words_;
            if (!fill_words(runs, {HudPortValue{"h", hud_shortest(now.season_light[index])}}, failed) ||
                !light_.set(runs, failed)) {
                return false;
            }
            UiTextPlace light;
            light.at = label + ::godot::Vector2(0.0f, static_cast<float>(light_dy_));
            light.size = light_size_;
            light.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
            light.color = soft_;
            light.opacity = 1.0;
            light.fade = true;
            light.delay = light_delay_;
            light.elapsed = now.elapsed;
            light.still = now.still;
            light_.draw(canvas, frame, light);
        }
    }

    const double today = share_angle(now.year_share);
    canvas.draw_line(UiRing::point(centre, radius_ + today_inner_, today),
                     UiRing::point(centre, radius_ + today_outer_, today), time_, static_cast<float>(today_line_),
                     true);
    return true;
}

bool YearRing::draw_months(UiPen& canvas, const ::godot::Transform2D& frame, const YearRingFrame& now,
                           std::string& failed) {
    if (marks_ < 1.0) {
        failed = "hud_year_marks";
        return false;
    }
    const ::godot::Vector2 centre(static_cast<float>(x_), static_cast<float>(y_));
    const uint32_t token_marks = static_cast<uint32_t>(marks_);
    const uint32_t marks = token_marks < now.months_per_year ? token_marks : now.months_per_year;
    const double half = GODOT_RING_TURN_DEG / static_cast<double>(marks) / 2.0;
    double stretch_from = 0.0;
    double stretch_to = GODOT_RING_TURN_DEG;
    for (uint32_t mark = 0; mark < marks; ++mark) {
        const uint32_t lun = mark_lun(mark, marks, now.months_per_year);
        const uint32_t next_lun = mark_lun(mark + 1U, marks, now.months_per_year);
        const double angle = month_angle(static_cast<double>(lun), now.lun_share);
        const bool current = now.lun >= lun && now.lun < next_lun;
        if (current) {
            stretch_from = angle;
            stretch_to = month_angle(static_cast<double>(next_lun), now.lun_share);
        }
        canvas.draw_line(UiRing::point(centre, radius_ + tick_inner_, angle),
                         UiRing::point(centre, radius_ + tick_outer_, angle), faint_,
                         static_cast<float>(line_hair_), true);
        if (!number_.set(hud_whole(lun), failed)) {
            return false;
        }
        UiTextPlace number;
        number.at = UiRing::point(centre, radius_ - number_inset_, angle + half) +
                    ::godot::Vector2(0.0f, static_cast<float>(number_dy_));
        number.size = number_size_;
        number.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
        number.color = current ? time_ : soft_;
        number.opacity = 1.0;
        number.elapsed = now.elapsed;
        number.still = now.still;
        number_.draw(canvas, frame, number);
    }

    UiArcStyle style;
    style.opacity = lun_alpha_;
    style.drawn = 1.0;
    ring_.arc(canvas, centre, radius_ - lun_inset_, stretch_from + lun_trim_, stretch_to - lun_trim_, time_,
              lun_width_, style);
    return true;
}

bool YearRing::draw_trail(UiPen& canvas, const ::godot::Transform2D& frame, const YearRingFrame& now,
                          std::string& failed) {
    // The dots stand in full from the frame; who filled it, flora.trail.v1 or the bridge, the ring never asks
    // (VivariumView::fill_hud logs N0450 on its edge).
    const ::godot::Vector2 centre(static_cast<float>(x_), static_cast<float>(y_));
    for (const YearRingEvent& event : now.events) {
        ::godot::Color tone;
        if (!theme_->tone(event.tone, tone)) {
            failed = "YearRing: the event's domain tone '" + event.tone + "' has no DOMAIN_* colour";
            return false;
        }
        ring_.disc(canvas, UiRing::point(centre, radius_ - event_inset_, share_angle(event.year_share)),
                   event_radius_, tone);
    }
    return true;
}

bool YearRing::draw_next(UiPen& canvas, const ::godot::Transform2D& frame, const YearRingFrame& now,
                         std::string& failed) {
    // The mark stands from the frame; who filled it, flora.patch.v1 or the bridge, the ring never asks
    // (VivariumView::bridge_patch logs N0454 on its edge).
    if (!now.next_due) {
        return true;
    }
    if (now.next_stage >= stage_words_.size()) {
        failed = "YearRing: the stage " + hud_whole(now.next_stage) + " has no word in texts.hud.stages";
        return false;
    }

    const ::godot::Vector2 centre(static_cast<float>(x_), static_cast<float>(y_));
    const ::godot::Vector2 mark = UiRing::point(centre, radius_ + next_gap_, share_angle(now.next_share));
    ring_.circle(canvas, mark, next_radius_, life_, next_line_, 1.0);

    if (now.level >= level_light_) {
        if (!stage_.set(stage_words_[now.next_stage], failed)) {
            return false;
        }
        UiTextPlace word;
        word.at = mark + ::godot::Vector2(static_cast<float>(next_label_dx_), static_cast<float>(next_label_dy_));
        word.size = next_label_size_;
        word.anchor = GODOT_TEXT_ANCHOR_START;
        word.color = life_;
        word.opacity = 1.0;
        word.elapsed = now.elapsed;
        word.still = now.still;
        stage_.draw(canvas, frame, word);
    }
    return true;
}

bool YearRing::draw(UiPen& canvas, const ::godot::Transform2D& frame, const YearRingFrame& now,
                    std::string& failed) {
    if (now.months_per_year == 0 || now.lun == 0 || now.lun > now.months_per_year) {
        failed = "YearRing: the clock record must give the LUN of a year and a current LUN from 1 within it";
        return false;
    }
    if (!(now.lun_share > 0.0) || now.lun_share > 1.0) {
        failed = "YearRing: the day record must give the share of the year a whole LUN fills (CAL_LUN_SHARE)";
        return false;
    }
    canvas.draw_set_transform_matrix(frame);
    const bool drawn = draw_seasons(canvas, frame, now, failed) && draw_months(canvas, frame, now, failed) &&
                       draw_trail(canvas, frame, now, failed) && draw_next(canvas, frame, now, failed);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return drawn;
}

}  // namespace ase::adp::godot
