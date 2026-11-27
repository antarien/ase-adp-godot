/**
 * ASE GODOT ADAPTER - HUD ARC READOUT
 *
 * @file        godot_hud_arc_readout.cpp
 * @brief       ArcReadout: the SURA date and the next event with its deadline, see godot_hud_arc_readout.hpp
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, mockup #arcDate and #arcNext.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_arc_readout.hpp>
#include <ase/adp/godot/godot_host_record.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/hud/ring/types.hpp>
#include <ase/adp/godot/port/types.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

namespace {

/** A part of the SURA number: a whole count. */
std::string part(double value) {
    return hud_whole(static_cast<int64_t>(value));
}

}  // namespace

bool ArcReadout::bind(const UiTheme& theme, const ArcSign& sign, std::string& failed) {
    theme_ = &theme;
    sign_ = &sign;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_arc_readout_x", &x_},
             {"hud_arc_date_y", &date_y_},
             {"hud_arc_date_size", &date_size_},
             {"hud_arc_next_y", &next_y_},
             {"hud_arc_next_size", &next_size_},
             {"hud_arc_next_decimals", &decimals_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!theme.color("OVERLAY_INK", ink_) || !theme.words("arc.next", next_runs_) ||
        !theme.words("arc.next_days", next_days_runs_)) {
        failed = "OVERLAY_INK, texts.hud.arc.next, texts.hud.arc.next_days";
        return false;
    }
    period_words_.clear();
    for (int32_t code = GODOT_PERIOD_FIRST; code <= GODOT_PERIOD_LAST; ++code) {
        std::vector<HudRun> word;
        const std::string key = "arc.period.full." + std::to_string(code);
        if (!theme.words(key, word)) {
            failed = "texts.hud." + key;
            return false;
        }
        period_words_.push_back(word);
    }
    date_line_.clear();
    next_line_.clear();
    return date_.bind_data(theme, failed) && next_.bind_label(theme, failed);
}

bool ArcReadout::set_date(const SkyArcFrame& now, std::string& failed) {
    const double code = now.period - static_cast<double>(GODOT_PERIOD_FIRST);
    if (!whole(code) || static_cast<size_t>(code) >= period_words_.size()) {
        failed = "ArcReadout: the day period " + hud_shortest(now.period) + " lies outside GODOT_PERIOD_*";
        return false;
    }
    std::string name;
    if (!word_text(period_words_[static_cast<size_t>(code)], name, failed)) {
        return false;
    }
    const std::string gap = GODOT_ARC_SURA_PART_GAP;
    const std::string line = name + GODOT_ARC_SURA_GAP + part(now.year) + gap + part(now.epoch) + gap +
                             part(now.cycle) + gap + part(now.month) + gap + part(now.month_day) + gap + part(now.hour);
    if (line == date_line_) {
        return true;
    }
    date_line_ = line;
    return date_.set(line, failed);
}

bool ArcReadout::set_next(const SkyArcFrame& now, std::string& line, std::string& failed) {
    line.clear();
    if (now.next_kind.empty()) {
        next_line_.clear();
        return true;
    }
    std::vector<HudRun> word;
    std::string name;
    if (!sign_->word_of(now.next_kind, now.next_body, word, failed) || !word_text(word, name, failed) ||
        !sign_->tone_of(now.next_kind, now.next_body, next_tone_, failed)) {
        return false;
    }
    // From one whole TUMBRAE on the owner's day count names the deadline, below it the hours; the adapter
    // only picks the line, it never turns hours into days (mockup arcNext: dh < 30 HOR, else TUM).
    const bool days = now.next_in_d > 0.0;
    std::vector<HudRun> runs = days ? next_days_runs_ : next_runs_;
    const HudPortValue amount = days ? HudPortValue{"n", hud_shortest(now.next_in_d)}
                                     : HudPortValue{"h", hud_decimals(now.next_in_h, static_cast<int32_t>(decimals_))};
    if (!fill_words(runs, {HudPortValue{"word", name}, amount}, failed) || !word_text(runs, line, failed)) {
        return false;
    }
    if (line != next_line_) {
        next_line_ = line;
        return next_.set(runs, failed);
    }
    return true;
}

bool ArcReadout::draw(UiPen& canvas, const ::godot::Transform2D& frame, const SkyArcFrame& now,
                      std::vector<std::string>& flank, std::string& failed) {
    std::string changed;
    const std::string before = next_line_;
    const std::string date_before = date_line_;
    if (!set_date(now, failed) || !set_next(now, changed, failed)) {
        return false;
    }
    if (date_line_ != date_before) {
        flank.push_back(std::string(GODOT_ARC_LINE_DATE) + " period=" + hud_shortest(now.period) + " year=" +
                        hud_shortest(now.year) + " epoch=" + hud_shortest(now.epoch) + " cycle=" + hud_shortest(now.cycle) +
                        " month=" + hud_shortest(now.month) + " day=" + hud_shortest(now.month_day) + " hour=" +
                        hud_shortest(now.hour) + " text=" + date_line_);
    }
    if (next_line_ != before) {
        flank.push_back(std::string(GODOT_ARC_LINE_NEXT) + " kind=" + now.next_kind + " body=" + hud_shortest(now.next_body) +
                        " in_h=" + hud_decimals(now.next_in_h, static_cast<int32_t>(decimals_)) +
                        " in_d=" + hud_shortest(now.next_in_d));
    }
    UiTextPlace date;
    date.at = ::godot::Vector2(static_cast<float>(x_), static_cast<float>(date_y_));
    date.size = date_size_;
    date.anchor = GODOT_TEXT_ANCHOR_START;
    date.color = ink_;
    date.opacity = 1.0;
    date.elapsed = now.elapsed;
    date.still = now.still;
    date_.draw(canvas, frame, date);
    if (!next_line_.empty()) {
        UiTextPlace next = date;
        next.at = ::godot::Vector2(static_cast<float>(x_), static_cast<float>(next_y_));
        next.size = next_size_;
        next.color = next_tone_;
        next_.draw(canvas, frame, next);
    }
    return true;
}

}  // namespace ase::adp::godot
