/**
 * ASE GODOT ADAPTER - HUD UI LEADER LINE
 *
 * @file        godot_hud_ui_leader_line.cpp
 * @brief       Draws a leader line growing from its first point, or dashed across its knees
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_leader_line.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-09
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_leader_line.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/** The length of the polyline `points`. */
double polyline_length(const ::godot::PackedVector2Array& points) {
    double length = 0.0;
    for (int64_t index = 1; index < points.size(); ++index) {
        length += static_cast<double>(points[index - 1].distance_to(points[index]));
    }
    return length;
}

}  // namespace

bool UiLeaderLine::bind(const UiTheme& theme, std::string& failed) {
    if (!draw_.bind(theme, "hud_anim_draw", failed)) {
        return false;
    }
    if (!theme.measure("hud_line", width_)) {
        failed = "hud_line";
        return false;
    }
    if (!theme.measure("hud_line_dash", dash_) || dash_ <= 0.0) {
        failed = "hud_line_dash";
        return false;
    }
    if (!theme.measure("hud_line_gap", gap_) || gap_ < 0.0) {
        failed = "hud_line_gap";
        return false;
    }
    return true;
}

double UiLeaderLine::drawn(double elapsed, double delay, bool still) const {
    return draw_.progress(elapsed, delay, still);
}

void UiLeaderLine::draw(UiPen& canvas, const ::godot::PackedVector2Array& points,
                        const ::godot::Color& color, double width, double drawn) const {
    if (points.size() < 2) {
        return;
    }
    // .draw with pathLength 1: the stroke shows the share `drawn` of the whole length from the first point.
    double rest = polyline_length(points) * ase::math::clamp(drawn, 0.0, 1.0);
    if (rest <= 0.0) {
        return;
    }
    ::godot::PackedVector2Array shown;
    shown.push_back(points[0]);
    for (int64_t index = 1; index < points.size() && rest > 0.0; ++index) {
        const ::godot::Vector2 from = points[index - 1];
        const ::godot::Vector2 to = points[index];
        const double segment = static_cast<double>(from.distance_to(to));
        if (segment <= rest) {
            shown.push_back(to);
            rest -= segment;
            continue;
        }
        shown.push_back(from.lerp(to, static_cast<float>(rest / segment)));
        rest = 0.0;
    }
    canvas.draw_polyline(shown, color, static_cast<float>(width), true);
}

void UiLeaderLine::dashed(UiPen& canvas, const ::godot::PackedVector2Array& points,
                          const ::godot::Color& color) const {
    // stroke-dasharray dash gap over one polyline, the one dash rule of the HUD.
    svg_dash_points(canvas, points, color, width_, dash_, gap_);
}

}  // namespace ase::adp::godot
