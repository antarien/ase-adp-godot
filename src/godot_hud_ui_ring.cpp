/**
 * ASE GODOT ADAPTER - HUD UI RING
 *
 * @file        godot_hud_ui_ring.cpp
 * @brief       Draws the rings, arcs, stepped arcs and pulses of the HUD as the mockup's ARC and .pulse do
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, see godot_hud_ui_ring.hpp.
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

#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <ase/math/math.hpp>

#include <cstdint>

namespace ase::adp::godot {

namespace {

/** Godot's angle in radians (0 at +x, clockwise on the canvas) of the mockup's angle in degrees from the top. */
float canvas_angle(double degrees) {
    return static_cast<float>((degrees - GODOT_RING_TOP_DEG) / GODOT_RING_TURN_DEG) * ase::math::TWO_PI;
}

/** `color` with its alpha multiplied by `opacity`. */
::godot::Color faded(const ::godot::Color& color, double opacity) {
    ::godot::Color out = color;
    out.a = static_cast<float>(static_cast<double>(color.a) * opacity);
    return out;
}

/** The polyline points draw_arc gets for a sweep of `sweep_deg`: its share of a full turn, never fewer than the least. */
int32_t arc_points(double sweep_deg) {
    const int32_t share = static_cast<int32_t>(
        ase::math::ceil(static_cast<float>(GODOT_RING_POINTS_PER_TURN * sweep_deg / GODOT_RING_TURN_DEG)));
    return ase::math::max(share, GODOT_RING_POINTS_MIN);
}

}  // namespace

bool UiRing::bind(const UiTheme& theme, std::string& failed) {
    if (!draw_.bind(theme, "hud_anim_draw", failed) || !pulse_.bind(theme, "hud_anim_pulse", failed)) {
        return false;
    }
    if (!theme.measure("hud_anim_pulse_scale", pulse_scale_)) {
        failed = "hud_anim_pulse_scale";
        return false;
    }
    if (!theme.measure("hud_anim_pulse_alpha", pulse_alpha_)) {
        failed = "hud_anim_pulse_alpha";
        return false;
    }
    if (!theme.measure("hud_anim_still_pulse_alpha", still_alpha_)) {
        failed = "hud_anim_still_pulse_alpha";
        return false;
    }
    return true;
}

void UiRing::disc(UiPen& canvas, const ::godot::Vector2& centre, double radius,
                  const ::godot::Color& fill) const {
    canvas.draw_circle(centre, static_cast<float>(radius), fill);
}

void UiRing::circle(UiPen& canvas, const ::godot::Vector2& centre, double radius,
                    const ::godot::Color& color, double width, double opacity) const {
    canvas.draw_arc(centre, static_cast<float>(radius), 0.0f, ase::math::TWO_PI,
                    arc_points(GODOT_RING_TURN_DEG) + 1, faded(color, opacity), static_cast<float>(width), true);
}

void UiRing::arc(UiPen& canvas, const ::godot::Vector2& centre, double radius, double from_deg,
                 double to_deg, const ::godot::Color& color, double width, const UiArcStyle& style) const {
    // ARC: no path for a sweep under the least; arcP: a full turn cut just short of it.
    if (to_deg <= from_deg + GODOT_RING_MIN_DEG) {
        return;
    }
    double end_deg = to_deg;
    if (end_deg - from_deg >= GODOT_RING_FULL_DEG) {
        end_deg = from_deg + GODOT_RING_FULL_DEG;
    }
    // .draw: pathLength 1, stroke-dashoffset from 1 to 0 - the drawn share of the sweep from its start.
    const double sweep = (end_deg - from_deg) * ase::math::clamp(style.drawn, 0.0, 1.0);
    if (sweep <= 0.0) {
        return;
    }
    const ::godot::Color drawn_color = faded(color, style.opacity);
    const float start = canvas_angle(from_deg);
    const float end = canvas_angle(from_deg + sweep);
    canvas.draw_arc(centre, static_cast<float>(radius), start, end, arc_points(sweep), drawn_color,
                    static_cast<float>(width), true);
    if (style.round_cap) {
        // stroke-linecap round: a half-width disc on each end of the drawn arc.
        const float cap = static_cast<float>(width) * GODOT_HALF;
        const float reach = static_cast<float>(radius);
        canvas.draw_circle(centre + ::godot::Vector2(ase::math::cos(start), ase::math::sin(start)) * reach, cap,
                           drawn_color);
        canvas.draw_circle(centre + ::godot::Vector2(ase::math::cos(end), ase::math::sin(end)) * reach, cap,
                           drawn_color);
    }
}

double UiRing::drawn(double elapsed, double delay, bool still) const {
    return draw_.progress(elapsed, delay, still);
}

::godot::Vector2 UiRing::point(const ::godot::Vector2& centre, double radius, double degrees) {
    const float angle = canvas_angle(degrees);
    return centre + ::godot::Vector2(ase::math::cos(angle), ase::math::sin(angle)) * static_cast<float>(radius);
}

void UiRing::steps(UiPen& canvas, const ::godot::Vector2& centre, double radius, double from_deg,
                   double to_deg, double step_deg, const std::vector<::godot::Color>& colors, double width,
                   double opacity) const {
    UiArcStyle style;
    style.opacity = opacity;
    style.drawn = 1.0;
    for (size_t index = 0; index < colors.size(); ++index) {
        const double offset = static_cast<double>(index) * step_deg;
        arc(canvas, centre, radius, offset + from_deg, offset + to_deg, colors[index], width, style);
    }
}

void UiRing::pulse(UiPen& canvas, const ::godot::Vector2& centre, double radius,
                   const ::godot::Color& color, double width, double elapsed, bool still) const {
    if (still) {
        circle(canvas, centre, radius, color, width, still_alpha_);
        return;
    }
    // @keyframes pulse: scale 1 to hud_anim_pulse_scale, opacity hud_anim_pulse_alpha to 0, eased per turn;
    // the element scales, so the stroke grows with the radius.
    const double share = pulse_.cycle(elapsed);
    const double scale = 1.0 + (pulse_scale_ - 1.0) * share;
    const double opacity = pulse_alpha_ * (1.0 - share);
    circle(canvas, centre, radius * scale, color, width * scale, opacity);
}

}  // namespace ase::adp::godot
