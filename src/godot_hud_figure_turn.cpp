/**
 * ASE GODOT ADAPTER - HUD FIGURE TURN
 *
 * @file        godot_hud_figure_turn.cpp
 * @brief       FigureTurn - the arc of fig.turn on the ground before the feet, laid toward the eye of each frame
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.7 (VIVARIUM_PHASE_07_FIGURE_V1, turnArc, nearArc, drawHud fig.turn);
 *              godot_hud_figure_turn.hpp names every measure, colour and word.
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

#include <ase/adp/godot/godot_hud_figure_turn.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/hud/types.hpp>
#include <ase/math/math.hpp>

#include <cstdint>
#include <initializer_list>
#include <utility>
#include <vector>

namespace ase::adp::godot {

bool FigureTurn::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_figure_turn_radius", &radius_},
             {"hud_figure_turn_span", &span_},
             {"hud_figure_turn_segments", &segments_},
             {"hud_figure_turn_lift", &lift_},
             {"hud_figure_turn_line", &line_},
             {"hud_figure_turn_rest_alpha", &rest_alpha_},
             {"hud_figure_turn_lit_alpha", &lit_alpha_},
             {"hud_figure_turn_held_alpha", &held_alpha_},
             {"hud_figure_turn_chevron_size", &chevron_size_},
             {"hud_figure_turn_chevron_dx", &chevron_dx_},
             {"hud_figure_turn_chevron_dy", &chevron_dy_},
             {"hud_figure_turn_hit", &hit_},
             {"hud_figure_turn_yaw_per_unit", &yaw_per_unit_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!(radius_ > 0.0) || !(segments_ >= 1.0)) {
        failed = "hud_figure_turn_segments";  // an arc without reach or without a segment lies nowhere
        return false;
    }
    if (!theme.color("FIGURE_INK", ink_)) {
        failed = "FIGURE_INK";
        return false;
    }
    std::vector<HudRun> runs;
    for (const std::pair<UiTextBlock*, const char*>& slot : std::initializer_list<std::pair<UiTextBlock*, const char*>>{
             {&left_, "figure.turn.left_glyph"}, {&right_, "figure.turn.right_glyph"}}) {
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
    return true;
}

bool FigureTurn::lay(const UiAnchor& anchor, const ::godot::Vector3& feet, const ::godot::Vector3& eye) {
    // turnArc: the arc's middle points from the feet toward the eye, so it always lies at the bottom of the figure
    points_.clear();
    const float to_eye = ase::math::atan2(eye.x - feet.x, eye.z - feet.z);
    const int32_t count = static_cast<int32_t>(segments_);
    for (int32_t index = 0; index <= count; ++index) {
        const float angle = to_eye - static_cast<float>(span_) * 0.5f +
                            static_cast<float>(span_) * static_cast<float>(index) / static_cast<float>(count);
        const ::godot::Vector3 world(feet.x + ase::math::sin(angle) * static_cast<float>(radius_),
                                     feet.y + static_cast<float>(lift_),
                                     feet.z + ase::math::cos(angle) * static_cast<float>(radius_));
        ::godot::Vector2 point;
        if (!anchor.place(world, point)) {
            points_.clear();  // a point behind the camera: the arc does not stand in this frame
            return false;
        }
        points_.push_back(point);
    }
    return true;
}

void FigureTurn::clear() {
    points_.clear();
}

void FigureTurn::draw(UiPen& canvas, const ::godot::Transform2D& frame, bool lit, bool held) {
    if (points_.size() < 2) {
        return;
    }
    const double alpha = held ? held_alpha_ : (lit ? lit_alpha_ : rest_alpha_);
    ::godot::Color stroke = ink_;
    stroke.a *= static_cast<float>(alpha);
    const int64_t last = points_.size() - 1;
    canvas.draw_set_transform_matrix(frame);
    // stroke-linecap round on both ends of the arc, the one round stroke of the HUD (godot_hud_ui_stroke).
    svg_polyline_round(canvas, frame, points_, stroke, line_);
    canvas.draw_set_transform_matrix(::godot::Transform2D());

    // the chevrons: ‹ ends dx left of the first point, › starts dx right of the last, both dy under them
    UiTextPlace text;
    text.size = chevron_size_;
    text.color = ink_;
    text.opacity = alpha;
    text.still = true;
    text.at = points_[0] + ::godot::Vector2(static_cast<float>(-chevron_dx_), static_cast<float>(chevron_dy_));
    text.anchor = GODOT_TEXT_ANCHOR_END;
    left_.draw(canvas, frame, text);
    text.at = points_[last] + ::godot::Vector2(static_cast<float>(chevron_dx_), static_cast<float>(chevron_dy_));
    text.anchor = GODOT_TEXT_ANCHOR_START;
    right_.draw(canvas, frame, text);
}

bool FigureTurn::hit(const ::godot::Vector2& point) const {
    // nearArc: any point of the arc within hit
    for (int64_t index = 0; index < points_.size(); ++index) {
        if (static_cast<double>(point.distance_to(points_[index])) <= hit_) {
            return true;
        }
    }
    return false;
}

}  // namespace ase::adp::godot
