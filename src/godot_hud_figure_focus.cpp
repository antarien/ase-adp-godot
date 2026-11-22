/**
 * ASE GODOT ADAPTER - HUD FIGURE FOCUS
 *
 * @file        godot_hud_figure_focus.cpp
 * @brief       FigureFocus - the reach of the seat's figure, the comb soul.mode over its head, the spine of its soul
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.9 and 07.11 (VIVARIUM_PHASE_07_FIGURE_V1, fig.focus) in the
 *              grammar of Phase 06 zone Z2 without its ring; godot_hud_figure_focus.hpp names every measure and word.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_021
 * @category    ecs/module
 * @usecase     usecase:101554 - receive player input
 * @actor       Player Input Warden (PLAYINPWAR)
 * @created     2026-11-14
 * @modified    2026-11-14
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_figure_focus.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/hud/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <initializer_list>
#include <utility>
#include <vector>

namespace ase::adp::godot {

namespace {

constexpr double kCombOpaque = 1.0;  // the comb's glyph and word at the full opacity of their colour

}  // namespace

bool FigureFocus::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_figure_focus_anchor_share", &anchor_share_},
             {"hud_figure_focus_ring_share", &ring_share_},
             {"hud_figure_focus_soul_dx", &soul_dx_},
             {"hud_figure_focus_soul_label_dy", &soul_label_dy_},
             {"hud_figure_focus_soul_label_size", &soul_label_size_},
             {"hud_figure_focus_soul_value_dy", &soul_value_dy_},
             {"hud_figure_focus_soul_value_size", &soul_value_size_},
             {"hud_focus_radius", &radius_min_},
             {"hud_focus_leader_delay", &leader_delay_},
             {"hud_focus_title_delay", &label_delay_},
             {"hud_focus_subtitle_delay", &value_delay_},
             {"hud_fan_delay", &chip_delay_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!theme.color("DOMAIN_FLOW", flow_)) {
        failed = "DOMAIN_FLOW";
        return false;
    }

    std::vector<HudRun> runs;
    for (const std::pair<UiTextBlock*, const char*>& slot : std::initializer_list<std::pair<UiTextBlock*, const char*>>{
             {&soul_label_, "figure.focus.soul"},
             {&soul_player_, "figure.soul.sspc"},
             {&soul_ai_, "figure.soul.aipc"},
             {&comb_word_ai_, "figure.soul_mode.to_ai"},
             {&comb_word_self_, "figure.soul_mode.to_player"}}) {
        runs.clear();
        if (!slot.first->bind_label(theme, failed)) {
            return false;
        }
        if (!theme.words(slot.second, runs)) {
            failed = slot.second;
            return false;
        }
        if (!slot.first->set(runs, failed)) {
            return false;
        }
    }

    // the comb over the head and the spine right of the figure (07.11): every measure and colour is SHA's
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_figure_focus_comb_radius", &comb_radius_},
             {"hud_figure_focus_comb_gap", &comb_gap_},
             {"hud_figure_focus_comb_pad", &comb_pad_},
             {"hud_figure_focus_comb_line", &comb_line_},
             {"hud_figure_focus_comb_glyph_size", &comb_glyph_size_},
             {"hud_figure_focus_comb_word_dx", &comb_word_dx_},
             {"hud_figure_focus_comb_word_dy", &comb_word_dy_},
             {"hud_figure_focus_comb_word_size", &comb_word_size_},
             {"hud_figure_focus_comb_hit", &comb_hit_},
             {"hud_figure_focus_body_share", &body_share_},
             {"hud_figure_focus_leader_line", &leader_line_},
             {"hud_figure_focus_soul_text_dx", &soul_text_dx_},
             {"hud_figure_focus_spine_room", &spine_room_},
             {"hud_figure_focus_spine_low", &spine_low_},
             {"hud_figure_head_h", &head_bar_},
             {"hud_frame_width", &frame_w_},
             {"hud_frame_height", &frame_h_},
             {"hud_figure_switch_x", &switch_x_},
             {"hud_figure_switch_y", &switch_y_},
             {"hud_figure_switch_hit", &switch_hit_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{{"OVERLAY_DISC", &disc_},
                                                                       {"DOMAIN_SKY", &sky_},
                                                                       {"FIGURE_RAIL", &rail_},
                                                                       {"FIGURE_SOUL_LABEL", &soul_ink_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!bind_word_icon(theme, "figure.soul_mode.to_ai_glyph", comb_to_ai_, failed) ||
        !bind_word_icon(theme, "figure.soul_mode.to_player_glyph", comb_to_self_, failed)) {
        return false;
    }

    radius_ = 0.0;
    close();
    return leader_.bind(theme, failed);
}

void FigureFocus::lay(const FigureFocusFrame& now) {
    // r = max(hud_focus_radius, |anchor.y - head.y| * ring_share), the anchor anchor_share of the way to the head
    centre_ = now.foot + (now.head - now.foot) * static_cast<float>(anchor_share_);
    radius_ = ase::math::max(radius_min_, ase::math::abs(static_cast<double>(centre_.y - now.head.y)) * ring_share_);
}

bool FigureFocus::draw(UiPen& canvas, const ::godot::Transform2D& frame, const FigureFocusFrame& now,
                       std::string& failed) {
    lay(now);
    (void)failed;  // every word and glyph was bound in bind: nothing drawn here can miss

    // THE COMB: on the figure's axis, comb_gap above the head, held inside the frame - under the head bar, off the
    // frame's edges, left of cam.mode's square while it stands at the switch's height
    const double comb_r = comb_radius_;
    const double comb_top = head_bar_ + comb_pad_ + comb_r;
    const double comb_y = ase::math::min(
        ase::math::max(static_cast<double>(now.head.y) - comb_gap_ - comb_r, comb_top), frame_h_ - comb_pad_ - comb_r);
    const double comb_right = comb_y - comb_r < switch_y_ + switch_hit_ ? switch_x_ - switch_hit_ - comb_pad_ - comb_r
                                                                       : frame_w_ - comb_pad_ - comb_r;
    const double comb_x = ase::math::min(ase::math::max(static_cast<double>(centre_.x), comb_pad_ + comb_r), comb_right);
    const bool word_left = comb_x > frame_w_ * static_cast<double>(GODOT_HALF);  // the word stands toward the middle
    comb_ = ::godot::Vector2(static_cast<float>(comb_x), static_cast<float>(comb_y));
    comb_shown_ = true;
    const double drawn = leader_.drawn(now.elapsed, leader_delay_, now.still);

    canvas.draw_set_transform_matrix(frame);
    if (static_cast<double>(now.head.y) > comb_y + comb_r) {
        // the head stands below the comb (the comb was held under the head bar): a leader runs down to it
        ::godot::PackedVector2Array lead;
        lead.push_back(comb_ + ::godot::Vector2(0.0f, static_cast<float>(comb_r)));
        lead.push_back(::godot::Vector2(centre_.x, now.head.y));
        leader_.draw(canvas, lead, rail_, leader_line_, drawn);
    }
    ::godot::PackedVector2Array corners = hud_hex_points(comb_, comb_r);
    canvas.draw_colored_polygon(corners, disc_);
    corners.push_back(corners[0]);
    canvas.draw_polyline(corners, sky_, static_cast<float>(comb_line_), true);
    (now.led_self ? comb_to_ai_ : comb_to_self_).draw(canvas, frame, comb_, comb_glyph_size_, sky_, kCombOpaque);

    UiTextPlace text;
    text.at = ::godot::Vector2(static_cast<float>(word_left ? comb_x - comb_r - comb_word_dx_
                                                            : comb_x + comb_r + comb_word_dx_),
                               static_cast<float>(comb_y + comb_word_dy_));
    text.size = comb_word_size_;
    text.anchor = word_left ? GODOT_TEXT_ANCHOR_END : GODOT_TEXT_ANCHOR_START;
    text.color = sky_;
    text.opacity = kCombOpaque;
    text.fade = true;
    text.delay = chip_delay_;
    text.elapsed = now.elapsed;
    text.still = now.still;
    (now.led_self ? comb_word_ai_ : comb_word_self_).draw(canvas, frame, text);

    // THE SPINE: from body_share of the reach right of the axis out by soul_dx, kept soul's room off the right edge;
    // its row follows the body's middle, held between one comb under the head bar and spine_low over the dock
    const double spine_x0 = static_cast<double>(centre_.x) + radius_ * body_share_;
    const double spine_x1 = ase::math::min(spine_x0 + soul_dx_, frame_w_ - spine_room_);
    const double spine_y = ase::math::min(
        ase::math::max(static_cast<double>(centre_.y), head_bar_ + comb_pad_ + comb_r + comb_r), spine_low_);
    ::godot::PackedVector2Array spine;
    spine.push_back(::godot::Vector2(static_cast<float>(spine_x0), static_cast<float>(spine_y)));
    spine.push_back(::godot::Vector2(static_cast<float>(spine_x1), static_cast<float>(spine_y)));
    canvas.draw_set_transform_matrix(frame);
    leader_.draw(canvas, spine, rail_, leader_line_, drawn);

    text.at = ::godot::Vector2(static_cast<float>(spine_x1 + soul_text_dx_), static_cast<float>(spine_y + soul_label_dy_));
    text.size = soul_label_size_;
    text.anchor = GODOT_TEXT_ANCHOR_START;
    text.color = soul_ink_;
    text.delay = label_delay_;
    soul_label_.draw(canvas, frame, text);
    text.at = ::godot::Vector2(static_cast<float>(spine_x1 + soul_text_dx_), static_cast<float>(spine_y + soul_value_dy_));
    text.size = soul_value_size_;
    text.color = now.led_self ? flow_ : sky_;
    text.delay = value_delay_;
    (now.led_self ? soul_player_ : soul_ai_).draw(canvas, frame, text);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
    return true;
}

void FigureFocus::close() {
    comb_shown_ = false;
}

bool FigureFocus::hit_comb(const ::godot::Vector2& point) const {
    return comb_shown_ && hud_hex_contains(comb_, comb_radius_ + comb_hit_, point);
}

bool FigureFocus::hit_figure(const ::godot::Vector2& point) const {
    return radius_ > 0.0 && static_cast<double>(point.distance_to(centre_)) <= radius_;
}

bool FigureFocus::reach_place(::godot::Rect2& reach) const {
    if (!(radius_ > 0.0)) {
        return false;
    }
    const float half = static_cast<float>(radius_);
    reach = ::godot::Rect2(centre_.x - half, centre_.y - half, half * 2.0f, half * 2.0f);
    return true;
}

bool FigureFocus::comb_place(::godot::Rect2& comb) const {
    if (!comb_shown_) {
        return false;
    }
    const double outer = comb_radius_ + comb_hit_;
    const float side = static_cast<float>(outer * 2.0);
    comb = ::godot::Rect2(comb_.x - static_cast<float>(outer), comb_.y - static_cast<float>(outer), side, side);
    return true;
}

}  // namespace ase::adp::godot
