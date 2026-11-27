/**
 * ASE GODOT ADAPTER - HUD SKILL RINGS
 *
 * @file        godot_hud_skill_rings_draw.cpp
 * @brief       Draws the two skill rings DCD and ACD of the game HUD in the free corner right of the sky arc
 * @description MIG_ASE_SKILL_RSM step B5, usecase:101305 show skill progress, see godot_hud_skill_rings_draw.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_074
 * @category    ecs/module
 * @usecase     usecase:101305 - show skill progress
 * @actor       Skill Measurement Tracker (SKILMEATRA)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_skill_rings_draw.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/hud/ring/types.hpp>
#include <ase/adp/godot/rsm/types.hpp>

#include <godot_cpp/variant/vector2.hpp>

#include <cmath>
#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

namespace {

/** The point (x, y) in mockup units. */
::godot::Vector2 at(double x, double y) {
    return ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
}

/** True where `share` is a share of a turn: a number from 0 to 1. */
bool is_share(double share) {
    return std::isfinite(share) && share >= 0.0 && share <= 1.0;
}

}  // namespace

bool SkillRings::bind(const UiTheme& theme, std::string& failed) {
    if (!ring_.bind(theme, failed)) {
        return false;
    }
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_skill_ring_x", &x_},
             {"hud_skill_ring_dcd_y", &dcd_y_},
             {"hud_skill_ring_acd_y", &acd_y_},
             {"hud_skill_ring_r", &r_},
             {"hud_skill_ring_line", &line_},
             {"hud_skill_ring_word_dx", &word_dx_},
             {"hud_skill_ring_word_dy", &word_dy_},
             {"hud_skill_ring_word_size", &word_size_},
             {"hud_skill_ring_pct_dy", &pct_dy_},
             {"hud_skill_ring_pct_size", &pct_size_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!(r_ > 0.0) || !(line_ > 0.0)) {
        failed = "hud_skill_ring_r, hud_skill_ring_line: a ring needs a radius and a line above 0";
        return false;
    }
    for (const std::pair<const char*, ::godot::Color*>& tone : std::initializer_list<std::pair<const char*, ::godot::Color*>>{
             {"OVERLAY_INK_FAINT", &track_}, {"DOMAIN_TIME", &time_}, {"OVERLAY_INK", &ink_}}) {
        if (!theme.color(tone.first, *tone.second)) {
            failed = tone.first;
            return false;
        }
    }
    if (!theme.words("skill_ring.percent", percent_)) {
        failed = "skill_ring.percent";
        return false;
    }
    words_.assign(GODOT_RSM_RINGS, UiTextBlock());
    values_.assign(GODOT_RSM_RINGS, UiTextBlock());
    held_.assign(GODOT_RSM_RINGS, 0.0);
    const char* keys[GODOT_RSM_RINGS] = {"skill_ring.dcd", "skill_ring.acd"};
    for (uint32_t which = 0; which < GODOT_RSM_RINGS; ++which) {
        std::vector<HudRun> runs;
        if (!theme.words(keys[which], runs)) {
            failed = keys[which];
            return false;
        }
        if (!words_[which].bind_label(theme, failed) || !words_[which].set(runs, failed) ||
            !values_[which].bind_data(theme, failed)) {
            return false;
        }
    }
    theme_ = &theme;
    return true;
}

double SkillRings::shown(uint32_t which) const {
    return which < held_.size() ? held_[which] : 0.0;
}

bool SkillRings::draw(UiPen& canvas, const ::godot::Transform2D& frame, const SkillRingsFrame& now, double elapsed,
                      bool still, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.skill_rings.unbound";
        return false;
    }
    // THE RINGS STAND ALWAYS (operator 2026-11-25): a frame whose port names an entry sets both shares, a frame
    // without one keeps those drawn last; before the first learning order both stand at 0 %.
    if (now.known) {
        if (!is_share(now.dcd) || !is_share(now.acd)) {
            failed = "hud.skill_rings: dcd and acd are shares 0 to 1";
            return false;
        }
        held_[GODOT_RSM_RING_DCD] = now.dcd;
        held_[GODOT_RSM_RING_ACD] = now.acd;
    }
    UiTextPlace text;
    text.color = ink_;
    text.opacity = 1.0;
    text.elapsed = elapsed;
    text.still = still;
    for (uint32_t which = 0; which < GODOT_RSM_RINGS; ++which) {
        if (!ring(canvas, frame, which, text, failed)) {
            return false;
        }
    }
    canvas.draw_set_transform_matrix(frame);
    return true;
}

bool SkillRings::ring(UiPen& canvas, const ::godot::Transform2D& frame, uint32_t which, const UiTextPlace& text,
                      std::string& failed) {
    const double centre_y = which == GODOT_RSM_RING_DCD ? dcd_y_ : acd_y_;
    const ::godot::Vector2 centre = at(x_, centre_y);
    const double share = held_[which];

    // PercentageWheel: the full track, then the arc from the top clockwise over the share of a turn.
    canvas.draw_set_transform_matrix(frame);
    ring_.circle(canvas, centre, r_, track_, line_, 1.0);
    UiArcStyle style;
    style.opacity = 1.0;
    style.round_cap = false;
    style.drawn = 1.0;
    ring_.arc(canvas, centre, r_, 0.0, share * GODOT_RING_TURN_DEG, time_, line_, style);

    // The per cent inside the ring, the data face.
    std::vector<HudRun> percent = percent_;
    if (!fill_words(percent, {HudPortValue{"value", hud_percent(share)}}, failed) ||
        !values_[which].set(percent, failed)) {
        return false;
    }
    UiTextPlace value = text;
    value.at = at(x_, centre_y + pct_dy_);
    value.size = pct_size_;
    value.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    values_[which].draw(canvas, frame, value);

    // The word left of the ring, ending word_dx left of its centre.
    UiTextPlace word = text;
    word.at = at(x_ + word_dx_, centre_y + word_dy_);
    word.size = word_size_;
    word.anchor = GODOT_TEXT_ANCHOR_END;
    words_[which].draw(canvas, frame, word);
    return true;
}

}  // namespace ase::adp::godot
