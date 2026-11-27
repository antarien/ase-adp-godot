/**
 * ASE GODOT ADAPTER - HUD UI EDGE RAIL
 *
 * @file        godot_hud_ui_edge_rail.cpp
 * @brief       Draws the rail of signs at the left edge of the care view as the mockup's rail(sg) does
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_edge_rail.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_edge_rail.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/hud/ring/types.hpp>
#include <ase/adp/godot/hud/style/types.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>

namespace ase::adp::godot {

namespace {

/** Reads the token `name` into `out`; false with `name` in `failed`. */
bool read_token(const UiTheme& theme, const char* name, double& out, std::string& failed) {
    if (!theme.measure(name, out)) {
        failed = name;
        return false;
    }
    return true;
}

/** Reads the colour `name` into `out`; false with `name` in `failed`. */
bool read_color(const UiTheme& theme, const char* name, ::godot::Color& out, std::string& failed) {
    if (!theme.color(name, out)) {
        failed = name;
        return false;
    }
    return true;
}

}  // namespace

bool UiEdgeRail::bind(const UiTheme& theme, std::string& failed) {
    if (!line_.bind(theme, failed) || !ring_.bind(theme, failed) || !label_.bind_label(theme, failed) ||
        !system_.bind(theme, "ICON_HISTORY", failed)) {
        return false;
    }
    if (!read_color(theme, "OVERLAY_INK_FAINT", faint_, failed) ||
        !read_color(theme, "OVERLAY_INK_SOFT", soft_, failed) ||
        !read_color(theme, "DOMAIN_ALERT", alert_, failed) ||
        !read_color(theme, "OVERLAY_DOT_DARK", dot_on_, failed)) {
        return false;
    }
    if (!read_token(theme, "hud_rail_x", x_, failed) || !read_token(theme, "hud_rail_top", top_, failed) ||
        !read_token(theme, "hud_rail_bottom", bottom_, failed) ||
        !read_token(theme, "hud_rail_label_x", label_x_, failed) ||
        !read_token(theme, "hud_rail_label_y", label_y_, failed) ||
        !read_token(theme, "hud_rail_label_size", label_size_, failed) ||
        !read_token(theme, "hud_rail_label_turn", label_turn_, failed) ||
        !read_token(theme, "hud_rail_alert_x", alert_x_, failed) ||
        !read_token(theme, "hud_rail_alert_y", alert_y_, failed) ||
        !read_token(theme, "hud_rail_alert_w", alert_w_, failed) ||
        !read_token(theme, "hud_rail_alert_base", alert_base_, failed) ||
        !read_token(theme, "hud_rail_alert_step", alert_step_, failed) ||
        !read_token(theme, "hud_rail_dot_y", dot_y_, failed) ||
        !read_token(theme, "hud_rail_dot_step", dot_step_, failed) ||
        !read_token(theme, "hud_rail_dot_radius", dot_radius_, failed) ||
        !read_token(theme, "hud_rail_dot_on_radius", dot_on_radius_, failed) ||
        !read_token(theme, "hud_rail_dot_line", dot_line_, failed) ||
        !read_token(theme, "hud_rail_pulse_radius", pulse_radius_, failed) ||
        !read_token(theme, "hud_rail_hit_w", hit_w_, failed) ||
        !read_token(theme, "hud_rail_hit_h", hit_h_, failed) ||
        !read_token(theme, "hud_rail_system_y", system_y_, failed) ||
        !read_token(theme, "hud_rail_system_size", system_size_, failed) ||
        !read_token(theme, "hud_rail_system_hit", system_hit_, failed)) {
        return false;
    }
    theme_ = &theme;
    return true;
}

bool UiEdgeRail::set(const std::string& label, const std::vector<UiEdgeRailSign>& signs, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.edge_rail.unbound";
        return false;
    }
    signs_ = signs;
    return label_.set(label, failed);
}

void UiEdgeRail::draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still) {
    if (theme_ == nullptr) {
        return;
    }
    canvas.draw_set_transform_matrix(frame);
    // L([[x, top], [x, bottom]], {col: --hud3}): the line grows in from the top with class draw, no delay.
    ::godot::PackedVector2Array points;
    points.push_back(::godot::Vector2(static_cast<float>(x_), static_cast<float>(top_)));
    points.push_back(::godot::Vector2(static_cast<float>(x_), static_cast<float>(bottom_)));
    line_.draw(canvas, points, faint_, line_.width(), line_.drawn(elapsed, 0.0, still));

    // T(x, label_y, word, {a: end, tr: rotate(turn x label_y), col: --hud2}), on a column of its own
    // (hud_rail_label_x): turned, the word's glyph height lies across x, so on the rail's x it stood over the
    // rail and the alert bar (operator 2026-11-13: half a letter to the left, a clear gap to the line)
    UiTextPlace word;
    word.at = ::godot::Vector2(static_cast<float>(label_x_), static_cast<float>(label_y_));
    word.size = label_size_;
    word.anchor = GODOT_TEXT_ANCHOR_END;
    word.color = soft_;
    word.opacity = 1.0;
    word.elapsed = elapsed;
    word.still = still;
    word.rotate_deg = label_turn_;
    word.pivot = word.at;
    label_.draw(canvas, frame, word);

    canvas.draw_set_transform_matrix(frame);
    double alerts = 0.0;
    for (const UiEdgeRailSign& sign : signs_) {
        alerts += sign.alert ? 1.0 : 0.0;
    }
    if (alerts > 0.0) {
        canvas.draw_rect(::godot::Rect2(static_cast<float>(alert_x_), static_cast<float>(alert_y_),
                                        static_cast<float>(alert_w_),
                                        static_cast<float>(alert_base_ + alerts * alert_step_)),
                         alert_);
    }
    double y = dot_y_;
    for (const UiEdgeRailSign& sign : signs_) {
        const ::godot::Vector2 centre(static_cast<float>(x_), static_cast<float>(y));
        const double radius = sign.opened ? dot_on_radius_ : dot_radius_;
        ring_.disc(canvas, centre, radius, sign.opened ? dot_on_ : sign.tone);
        ring_.circle(canvas, centre, radius, sign.tone, dot_line_, 1.0);
        if (sign.alert) {
            ring_.pulse(canvas, centre, pulse_radius_, sign.tone, line_.width(), elapsed, still);
        }
        y += dot_step_;
    }
    // G(x, system_y, 'HISTORY', system_size, --hud2).
    system_.draw(canvas, frame, ::godot::Vector2(static_cast<float>(x_), static_cast<float>(system_y_)), system_size_,
                 soft_, 1.0);
}

bool UiEdgeRail::hit_sign(const ::godot::Vector2& point, int32_t& index) const {
    // The one target per sign is sign_place(); the place line names the same rect a tap is asked on.
    ::godot::Rect2 target;
    for (size_t place = 0; sign_place(place, target); ++place) {
        if (point.x >= target.position.x && point.x <= target.get_end().x && point.y >= target.position.y &&
            point.y <= target.get_end().y) {
            index = static_cast<int32_t>(place);
            return true;
        }
    }
    return false;
}

bool UiEdgeRail::sign_place(size_t index, ::godot::Rect2& place) const {
    // rect x=0 y=dot-hit_h/2 width=hit_w height=hit_h: from the frame's left edge, centred on the dot.
    if (index >= signs_.size()) {
        return false;
    }
    const double y = dot_y_ + dot_step_ * static_cast<double>(index);
    const double half = hit_h_ * GODOT_HALF;
    place = ::godot::Rect2(0.0f, static_cast<float>(y - half), static_cast<float>(hit_w_), static_cast<float>(hit_h_));
    return true;
}

bool UiEdgeRail::hit_system(const ::godot::Vector2& point) const {
    const ::godot::Rect2 place = system_place();
    return point.x >= place.position.x && point.x <= place.get_end().x && point.y >= place.position.y &&
           point.y <= place.get_end().y;
}

::godot::Rect2 UiEdgeRail::system_place() const {
    // rect x=0 y=system_y-hit/2 width=hit height=hit.
    const double half = system_hit_ * GODOT_HALF;
    return ::godot::Rect2(0.0f, static_cast<float>(system_y_ - half), static_cast<float>(system_hit_),
                          static_cast<float>(system_hit_));
}

}  // namespace ase::adp::godot
