/**
 * ASE GODOT ADAPTER - HUD FIGURE ZOOM
 *
 * @file        godot_hud_figure_zoom.cpp
 * @brief       FigureZoom - the rail of cam.zoom, its scale, its knob at the camera's distance and the metres
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.6 (VIVARIUM_PHASE_07_FIGURE_V1, zoomY, zoomAt, drawHud cam.zoom);
 *              godot_hud_figure_zoom.hpp names every measure, colour and word and the port the rail's metres go through.
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

#include <ase/adp/godot/godot_hud_figure_zoom.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/hud/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>

#include <cstdint>
#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

namespace {

/**
 * The rail as one capsule from `from` down to `to`, `width` wide with round ends, as stroke-linecap round draws a
 * vertical line in the mockup; one outline, so its translucent ink never lies twice over itself.
 */
void draw_capsule(UiPen& canvas, const ::godot::Vector2& from, const ::godot::Vector2& to, double width,
                  const ::godot::Color& color) {
    const int32_t steps = static_cast<int32_t>(GODOT_RING_POINTS_PER_TURN) / 2;
    const float reach = static_cast<float>(width) * 0.5f;
    ::godot::PackedVector2Array outline;
    for (int32_t index = 0; index <= steps; ++index) {
        // the near end: from the left over the top to the right
        const float angle = ase::math::PI + ase::math::PI * static_cast<float>(index) / static_cast<float>(steps);
        outline.push_back(from + ::godot::Vector2(ase::math::cos(angle), ase::math::sin(angle)) * reach);
    }
    for (int32_t index = 0; index <= steps; ++index) {
        // the far end: from the right under the foot to the left
        const float angle = ase::math::PI * static_cast<float>(index) / static_cast<float>(steps);
        outline.push_back(to + ::godot::Vector2(ase::math::cos(angle), ase::math::sin(angle)) * reach);
    }
    canvas.draw_colored_polygon(outline, color);
}

}  // namespace

bool FigureZoom::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_figure_zoom_x", &x_},
             {"hud_figure_zoom_top", &top_},
             {"hud_figure_zoom_length", &length_},
             {"hud_figure_zoom_hit_x", &hit_x_},
             {"hud_figure_zoom_hit_pad", &hit_pad_},
             {"hud_figure_zoom_pinch_min", &pinch_min_},
             {"hud_figure_zoom_min", &min_},
             {"hud_figure_zoom_max", &max_},
             {"hud_figure_zoom_face_share", &face_share_},
             {"hud_figure_zoom_face_distance", &face_distance_},
             {"hud_figure_zoom_line", &line_},
             {"hud_figure_zoom_sign_size", &sign_size_},
             {"hud_figure_zoom_sign_alpha", &sign_alpha_},
             {"hud_figure_zoom_plus_dy", &plus_dy_},
             {"hud_figure_zoom_minus_dy", &minus_dy_},
             {"hud_figure_zoom_knob_radius", &knob_radius_},
             {"hud_figure_zoom_knob_held_radius", &knob_held_radius_},
             {"hud_figure_zoom_knob_alpha", &knob_alpha_},
             {"hud_figure_zoom_label_dx", &label_dx_},
             {"hud_figure_zoom_label_dy", &label_dy_},
             {"hud_figure_zoom_label_size", &label_size_},
             {"hud_figure_zoom_label_alpha", &label_alpha_},
             {"hud_figure_zoom_label_held_alpha", &label_held_alpha_},
             {"hud_figure_zoom_label_digits", &label_digits_},
             {"hud_figure_zoom_name_x", &name_x_},
             {"hud_figure_zoom_name_y", &name_y_},
             {"hud_figure_zoom_name_size", &name_size_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    // a scale whose ends meet, or whose face share is no share, maps no distance to a height
    if (!(length_ > 0.0) || !(min_ > face_distance_) || !(max_ > min_) || !(face_distance_ > 0.0) ||
        !(face_share_ > 0.0) || !(face_share_ < 1.0)) {
        failed = "hud_figure_zoom_min";
        return false;
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{
             {"FIGURE_INK", &ink_}, {"FIGURE_RAIL", &rail_}, {"FIGURE_ZONE_ORBIT_LABEL", &name_ink_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    std::vector<HudRun> runs;
    for (const std::pair<UiTextBlock*, const char*>& slot : std::initializer_list<std::pair<UiTextBlock*, const char*>>{
             {&near_, "figure.zoom.near"}, {&far_, "figure.zoom.far"}, {&name_, "figure.zone.zoom"}}) {
        runs.clear();
        if (!slot.first->bind_data(theme, failed)) {
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
    if (!metres_.bind_data(theme, failed)) {
        return false;
    }
    if (!theme.words("figure.zoom.metres", metres_runs_)) {
        failed = "figure.zoom.metres";
        return false;
    }
    metres_shown_.clear();
    return true;
}

double FigureZoom::y_of(double distance) const {
    // zoomY: the lower share logarithmic from max to min, the top share linear from min to the face
    double share = 0.0;
    if (distance >= min_) {
        share = face_share_ + (1.0 - face_share_) *
                                  static_cast<double>(ase::math::log(static_cast<float>(distance / min_))) /
                                  static_cast<double>(ase::math::log(static_cast<float>(max_ / min_)));
    } else {
        share = face_share_ * (distance - face_distance_) / (min_ - face_distance_);
    }
    return top_ + length_ * share;
}

double FigureZoom::distance_at(double y) const {
    // zoomAt: the height held on the rail, then the inverse of zoomY's two pieces
    const double share = ase::math::min(1.0, ase::math::max(0.0, (y - top_) / length_));
    if (share >= face_share_) {
        return min_ * static_cast<double>(ase::math::pow(static_cast<float>(max_ / min_),
                                                         static_cast<float>((share - face_share_) / (1.0 - face_share_))));
    }
    return face_distance_ + (min_ - face_distance_) * share / face_share_;
}

bool FigureZoom::draw(UiPen& canvas, const ::godot::Transform2D& frame, double distance, bool held,
                      std::string& failed) {
    const float x = static_cast<float>(x_);
    const ::godot::Vector2 near_end(x, static_cast<float>(top_));
    const ::godot::Vector2 far_end(x, static_cast<float>(top_ + length_));
    const double knob_y = y_of(distance);

    canvas.draw_set_transform_matrix(frame);
    draw_capsule(canvas, near_end, far_end, line_, rail_);
    ::godot::Color knob = ink_;
    knob.a *= static_cast<float>(knob_alpha_);
    canvas.draw_circle(::godot::Vector2(x, static_cast<float>(knob_y)),
                       static_cast<float>(held ? knob_held_radius_ : knob_radius_), knob, true);
    canvas.draw_set_transform_matrix(::godot::Transform2D());

    UiTextPlace text;
    text.still = true;
    text.color = ink_;
    text.size = sign_size_;
    text.opacity = sign_alpha_;
    text.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    text.at = near_end + ::godot::Vector2(0.0f, static_cast<float>(plus_dy_));
    near_.draw(canvas, frame, text);
    text.at = far_end + ::godot::Vector2(0.0f, static_cast<float>(minus_dy_));
    far_.draw(canvas, frame, text);

    // the metres beside the knob: set again only when the written distance changed
    const std::string written = hud_decimals(distance, static_cast<int32_t>(label_digits_));
    if (written != metres_shown_) {
        std::vector<HudRun> runs = metres_runs_;
        if (!fill_words(runs, {HudPortValue{"value", written}}, failed) || !metres_.set(runs, failed)) {
            return false;
        }
        metres_shown_ = written;
    }
    text.size = label_size_;
    text.opacity = held ? label_held_alpha_ : label_alpha_;
    text.anchor = GODOT_TEXT_ANCHOR_END;
    text.at = ::godot::Vector2(static_cast<float>(x_ - label_dx_), static_cast<float>(knob_y + label_dy_));
    metres_.draw(canvas, frame, text);

    text.size = name_size_;
    text.color = name_ink_;
    text.opacity = 1.0;
    text.at = ::godot::Vector2(static_cast<float>(name_x_), static_cast<float>(name_y_));
    name_.draw(canvas, frame, text);
    return true;
}

bool FigureZoom::hit(const ::godot::Vector2& point) const {
    const double y = static_cast<double>(point.y);
    return static_cast<double>(point.x) >= hit_x_ && y >= top_ - hit_pad_ && y <= top_ + length_ + hit_pad_;
}

::godot::Rect2 FigureZoom::place() const {
    return ::godot::Rect2(static_cast<float>(hit_x_), static_cast<float>(top_), static_cast<float>((x_ - hit_x_) * 2.0),
                          static_cast<float>(length_));
}

bool FigureZoom::pinch(double from, double spread_from, double spread, double& out) const {
    // pinch: zoomT = clampZ(dist0 * d0 / d), only while the fingers stand further apart than the minimum
    if (!(spread > pinch_min_) || !(spread_from > pinch_min_)) {
        return false;
    }
    out = ase::math::min(max_, ase::math::max(face_distance_, from * spread_from / spread));
    return true;
}

}  // namespace ase::adp::godot
