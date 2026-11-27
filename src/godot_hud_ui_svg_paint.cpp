/**
 * ASE GODOT ADAPTER - HUD UI SVG PAINT
 *
 * @file        godot_hud_ui_svg_paint.cpp
 * @brief       Fill, stroke and dashed stroke of a flattened outline, as SVG paints a path
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_svg_paint.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-10
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_stroke.hpp>
#include <ase/adp/godot/hud/sprite/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/packed_color_array.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/** The points of `outline` as one polyline: back onto its first point when it is closed. */
::godot::PackedVector2Array stroke_points(const UiSvgOutline& outline) {
    ::godot::PackedVector2Array points = outline.points;
    if (outline.closed && points.size() > 1) {
        points.push_back(points[0]);
    }
    return points;
}

}  // namespace

void svg_fill(UiPen& canvas, const UiSvgOutline& outline, const ::godot::Color& color) {
    ::godot::PackedVector2Array points = outline.points;
    // A closed outline whose last point came back onto its first carries it once.
    if (points.size() > 1 &&
        points[points.size() - 1].distance_to(points[0]) < static_cast<float>(GODOT_ITEM_POINT_EPSILON)) {
        points.remove_at(points.size() - 1);
    }
    if (points.size() < GODOT_ITEM_POLYGON_MIN) {
        return;
    }
    ::godot::PackedColorArray colors;
    colors.push_back(color);
    canvas.draw_polygon(points, colors);
}

void svg_stroke(UiPen& canvas, const UiSvgOutline& outline, const ::godot::Color& color, double width) {
    const ::godot::PackedVector2Array points = stroke_points(outline);
    if (points.size() < 2 || !(width > 0.0)) {
        return;
    }
    canvas.draw_polyline(points, color, static_cast<float>(width), true);
}

void svg_stroke_round(UiPen& canvas, const ::godot::Transform2D& frame, const UiSvgOutline& outline,
                      const ::godot::Color& color, double width) {
    if (outline.closed) {
        // A closed outline has no end: stroke-linecap does not reach it, and its knees keep the path's miter.
        svg_stroke(canvas, outline, color, width);
        return;
    }
    ui_stroke(canvas, frame, outline.points, false, color, width);
}

void svg_stroke_joined(UiPen& canvas, const ::godot::Transform2D& frame, const UiSvgOutline& outline,
                       const ::godot::Color& color, double width) {
    ui_stroke(canvas, frame, outline.points, outline.closed, color, width);
}

void svg_polyline_round(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::PackedVector2Array& points,
                        const ::godot::Color& color, double width) {
    ui_stroke(canvas, frame, points, false, color, width);
}

void svg_stroke_dashed(UiPen& canvas, const UiSvgOutline& outline, const ::godot::Color& color,
                       double width, double dash, double gap) {
    svg_dash_points(canvas, stroke_points(outline), color, width, dash, gap);
}

void svg_dash_points(UiPen& canvas, const ::godot::PackedVector2Array& points,
                     const ::godot::Color& color, double width, double dash, double gap) {
    if (!(dash > 0.0) || gap < 0.0 || !(width > 0.0)) {
        return;
    }
    // stroke-dasharray dash gap over one polyline: the pattern's phase carries over each knee. Every dash, and each
    // piece of a dash that crosses a knee, is one pair of ends, laid out first and read through one pointer each way
    // instead of one call over the GDExtension border per point (P0692, as UiPen::moved reads its run, P0690).
    const double period = dash + gap;
    const int64_t count = points.size();
    const ::godot::Vector2* run = points.ptr();
    // A segment holds at most one dash per period, plus one cut at each of its ends.
    int64_t most = 0;
    for (int64_t index = 1; index < count; ++index) {
        most += static_cast<int64_t>(static_cast<double>(run[index - 1].distance_to(run[index])) / period) + 2;
    }
    if (most == 0) {
        return;
    }
    ::godot::PackedVector2Array pairs;
    pairs.resize(most * 2);
    ::godot::Vector2* out = pairs.ptrw();
    int64_t laid = 0;
    double phase = 0.0;
    for (int64_t index = 1; index < count; ++index) {
        const ::godot::Vector2 from = run[index - 1];
        const ::godot::Vector2 to = run[index];
        const double segment = static_cast<double>(from.distance_to(to));
        double along = 0.0;
        while (along < segment) {
            const bool on = phase < dash;
            const double left_in_phase = on ? dash - phase : period - phase;
            const double step = ase::math::min(left_in_phase, segment - along);
            if (on && laid + 2 <= most * 2) {
                out[laid] = from.lerp(to, static_cast<float>(along / segment));
                out[laid + 1] = from.lerp(to, static_cast<float>((along + step) / segment));
                laid += 2;
            }
            along += step;
            phase += step;
            if (phase >= period) {
                phase -= period;
            }
        }
    }
    if (laid < 2) {
        return;
    }
    // The pairs laid are handed to the pen at once: one canvas call for every dash of the stroke (UiPen::draw_multiline).
    pairs.resize(laid);
    canvas.draw_multiline(pairs, color, static_cast<float>(width), true);
}

}  // namespace ase::adp::godot
