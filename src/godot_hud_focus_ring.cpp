/**
 * ASE GODOT ADAPTER - HUD FOCUS RING
 *
 * @file        godot_hud_focus_ring.cpp
 * @brief       The ring of the focus: disc, ticks, the arcs of soil water and growth, their numbers, the stage glyph
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, see godot_hud_focus_ring.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_focus_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool FocusRing::bind(const UiTheme& theme, std::string& failed) {
    drawn_ = false;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_focus_radius", &radius_},
             {"hud_focus_ring_offset_min", &offset_min_},
             {"hud_focus_ring_offset_share", &offset_share_},
             {"hud_focus_ring_height_max", &height_max_},
             {"hud_focus_ring_fill_alpha", &fill_alpha_},
             {"hud_focus_tick_count", &tick_count_},
             {"hud_focus_tick_step", &tick_step_},
             {"hud_focus_tick_inner", &tick_inner_},
             {"hud_focus_tick_outer", &tick_outer_},
             {"hud_focus_tick_outer_major", &tick_outer_major_},
             {"hud_focus_tick_major_every", &tick_major_every_},
             {"hud_focus_arc_width", &arc_width_},
             {"hud_focus_arc_delay", &arc_delay_},
             {"hud_focus_arc_second_delay", &arc_second_delay_},
             {"hud_focus_plain_ring_line", &plain_line_},
             {"hud_focus_plain_ring_alpha", &plain_alpha_},
             {"hud_focus_number_gap", &number_gap_},
             {"hud_focus_number_dy", &number_dy_},
             {"hud_focus_number_second_dy", &number_second_dy_},
             {"hud_focus_number_size", &number_size_},
             {"hud_focus_stage_glyph_size", &glyph_size_},
             {"hud_focus_soil_dry_below", &soil_dry_},
             {"hud_line", &line_},
             {"hud_line_hair", &hair_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_DISC", &disc_},
                                                                       {"OVERLAY_INK_FAINT", &faint_},
                                                                       {"DOMAIN_WATER", &water_},
                                                                       {"DOMAIN_LIFE", &life_},
                                                                       {"DOMAIN_ALERT", &alert_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    // The ring's ground is the disc's colour at the focus's own alpha (rgba(6,10,12,.25) of the mockup).
    disc_.a = static_cast<float>(fill_alpha_);

    // One glyph per stage the view knows, never a count of this unit.
    stage_glyphs_.clear();
    for (uint8_t stage = 0; stage < GODOT_STAGE_COUNT; ++stage) {
        UiIcon glyph;
        if (!bind_word_icon(theme, "stage_glyph." + std::to_string(static_cast<uint32_t>(stage)), glyph, failed)) {
            return false;
        }
        stage_glyphs_.push_back(glyph);
    }

    // The thresholds of perception are the spec's levels, never a number of this unit.
    const std::vector<uint32_t>& levels = theme.spec().levels;
    if (levels.size() < 3) {
        failed = "hud.perception.levels";
        return false;
    }
    level_bars_ = levels[1];
    level_numbers_ = levels[2];

    return bind_word_icon(theme, "stage_glyph_dead", dead_glyph_, failed) && ring_.bind(theme, failed) &&
           water_number_.bind_data(theme, failed) && growth_number_.bind_data(theme, failed);
}

void FocusRing::draw_ticks(UiPen& canvas, const ::godot::Transform2D& frame) const {
    canvas.draw_set_transform_matrix(frame);
    ring_.disc(canvas, centre_, radius_, disc_);
    ring_.circle(canvas, centre_, radius_, faint_, line_, 1.0);
    const int64_t ticks = static_cast<int64_t>(tick_count_);
    const int64_t major = static_cast<int64_t>(tick_major_every_);
    for (int64_t tick = 0; tick < ticks; ++tick) {
        const double degrees = static_cast<double>(tick) * tick_step_;
        const double outer = (major > 0 && tick % major == 0) ? tick_outer_major_ : tick_outer_;
        ::godot::PackedVector2Array mark;
        mark.push_back(UiRing::point(centre_, radius_ + tick_inner_, degrees));
        mark.push_back(UiRing::point(centre_, radius_ + outer, degrees));
        canvas.draw_polyline(mark, faint_, static_cast<float>(hair_), true);
    }
}

bool FocusRing::draw(UiPen& canvas, const ::godot::Transform2D& frame, const FocusRingFrame& now,
                     std::string& failed) {
    if (now.stage >= GODOT_STAGE_COUNT) {
        failed = "FocusRing: stage code " + std::to_string(static_cast<uint32_t>(now.stage)) +
                 " outside GODOT_STAGE_*";
        return false;
    }
    // ry = by - max(14, H * 0.3), H no higher than the mockup's grown tree.
    const double lift = ase::math::max(offset_min_, ase::math::min(now.height, height_max_) * offset_share_);
    centre_ = ::godot::Vector2(now.trunk.x, static_cast<float>(now.trunk.y - lift));
    drawn_ = true;
    draw_ticks(canvas, frame);

    // Two arcs from perception levels[1] on, one plain ring below it.
    const double half = GODOT_RING_TURN_DEG * static_cast<double>(GODOT_HALF);
    if (now.level >= level_bars_) {
        UiArcStyle water;
        water.opacity = 1.0;
        water.drawn = ring_.drawn(now.elapsed, arc_delay_, now.still);
        ring_.arc(canvas, centre_, radius_, half, half + half * ase::math::clamp(now.soil_rel, 0.0, 1.0), water_,
                  arc_width_, water);
        UiArcStyle growth;
        growth.opacity = 1.0;
        growth.drawn = ring_.drawn(now.elapsed, arc_second_delay_, now.still);
        ring_.arc(canvas, centre_, radius_, 0.0, half * ase::math::clamp(now.growth_share, 0.0, 1.0), life_,
                  arc_width_, growth);
    } else {
        ring_.circle(canvas, centre_, radius_, now.soil_rel < soil_dry_ ? alert_ : life_, plain_line_,
                     plain_alpha_);
    }

    if (now.level >= level_numbers_ && !draw_numbers(canvas, frame, now, failed)) {
        return false;
    }

    // G(TX, ry, stage glyph, size, life) or the dead tree in the alert tone.
    UiIcon& glyph = now.dead ? dead_glyph_ : stage_glyphs_[now.stage];
    glyph.draw(canvas, frame, centre_, glyph_size_, now.dead ? alert_ : life_, 1.0);
    return true;
}

bool FocusRing::draw_numbers(UiPen& canvas, const ::godot::Transform2D& frame,
                             const FocusRingFrame& now, std::string& failed) {
    const std::string water = hud_percent(now.soil_rel);
    const std::string growth = hud_percent(now.growth_share);
    if (!water_number_.set(water, failed) || !growth_number_.set(growth, failed)) {
        return false;
    }

    // T(TX-R-gap, ry+dy, soil, d, end, water) and T(TX+R+gap, ry+second_dy, grow, d, life).
    UiTextPlace text;
    text.at = ::godot::Vector2(static_cast<float>(centre_.x - radius_ - number_gap_),
                               static_cast<float>(centre_.y + number_dy_));
    text.size = number_size_;
    text.anchor = GODOT_TEXT_ANCHOR_END;
    text.color = water_;
    text.opacity = 1.0;
    text.elapsed = now.elapsed;
    text.still = now.still;
    water_number_.draw(canvas, frame, text);
    text.at = ::godot::Vector2(static_cast<float>(centre_.x + radius_ + number_gap_),
                               static_cast<float>(centre_.y + number_second_dy_));
    text.anchor = GODOT_TEXT_ANCHOR_START;
    text.color = life_;
    growth_number_.draw(canvas, frame, text);
    return true;
}

bool FocusRing::hit(const ::godot::Vector2& point) const {
    return drawn_ && point.distance_to(centre_) <= static_cast<float>(radius_);
}

bool FocusRing::place(::godot::Rect2& out) const {
    const float reach = static_cast<float>(radius_);
    out = ::godot::Rect2(centre_ - ::godot::Vector2(reach, reach), ::godot::Vector2(reach, reach) * 2.0f);
    return drawn_;
}

}  // namespace ase::adp::godot
