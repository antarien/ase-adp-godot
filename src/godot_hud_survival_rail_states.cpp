/**
 * ASE GODOT ADAPTER - HUD SURVIVAL RAIL STATES
 *
 * @file        godot_hud_survival_rail_states.cpp
 * @brief       Draws the states of the body on the rail of signs SINAIS as the survival mockup's states(st) does
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U02, see godot_hud_survival_rail_states.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0528, N0530, carrier N0448
 * @created     2026-11-20
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_survival_rail_states.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>

#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>

#include <cstdint>
#include <initializer_list>

namespace ase::adp::godot {

namespace {

/** Reads the token `name` into `out`; false with `name` in `failed`. */
bool read_token(const UiTheme& theme, const std::string& name, double& out, std::string& failed) {
    if (!theme.measure(name, out)) {
        failed = name;
        return false;
    }
    return true;
}

/** Reads the path data of the token `name` into `outlines`; empty data reads as no part. */
bool read_part(const UiTheme& theme, const std::string& name, std::vector<UiSvgOutline>& outlines,
               std::string& failed) {
    ::godot::String text;
    if (!theme.token_text(name, text)) {
        failed = name;
        return false;
    }
    outlines.clear();
    if (text.is_empty()) {
        return true;
    }
    if (!read_svg_path(text.utf8().get_data(), outlines, failed)) {
        failed = name + ": " + failed;
        return false;
    }
    return true;
}

/** `color` with its alpha scaled by `share`: the fade of the group the mark stands in. */
::godot::Color faded(const ::godot::Color& color, double share) {
    ::godot::Color out = color;
    out.a = static_cast<float>(out.a * share);
    return out;
}

/** `outline` scaled by `scale` about `centre`: the path data of one unit placed as GL[name](x, y, k) places it. */
UiSvgOutline placed(const UiSvgOutline& outline, const ::godot::Vector2& centre, double scale) {
    UiSvgOutline out;
    out.closed = outline.closed;
    for (int64_t at = 0; at < outline.points.size(); ++at) {
        out.points.push_back(centre + outline.points[at] * static_cast<float>(scale));
    }
    return out;
}

}  // namespace

bool SurvivalRailStates::bind(const UiTheme& theme, std::string& failed) {
    if (!ring_.bind(theme, failed) || !fade_.bind(theme, "hud_anim_fade", failed)) {
        return false;
    }
    if (!theme.color("DOMAIN_ALERT", alert_)) {
        failed = "DOMAIN_ALERT";
        return false;
    }
    if (!read_token(theme, "hud_rail_x", x_, failed) || !read_token(theme, "hud_rail_alert_x", alert_x_, failed) ||
        !read_token(theme, "hud_rail_alert_w", alert_w_, failed) ||
        !read_token(theme, "hud_rail_alert_base", alert_base_, failed) ||
        !read_token(theme, "hud_rail_alert_step", alert_step_, failed) ||
        !read_token(theme, "hud_rail_state_alert_y", alert_y_, failed) ||
        !read_token(theme, "hud_rail_state_y", y_, failed) || !read_token(theme, "hud_rail_state_step", step_, failed) ||
        !read_token(theme, "hud_rail_state_radius", radius_, failed) ||
        !read_token(theme, "hud_rail_state_line", dot_line_, failed) ||
        !read_token(theme, "hud_rail_state_pulse_radius", pulse_radius_, failed) ||
        !read_token(theme, "hud_rail_state_pulse_line", pulse_line_, failed) ||
        !read_token(theme, "hud_rail_state_glyph_x", glyph_x_, failed) ||
        !read_token(theme, "hud_rail_state_glyph_size", glyph_size_, failed) ||
        !read_token(theme, "hud_survival_glyph_line", glyph_line_, failed)) {
        return false;
    }
    if (!bind_state(theme, poison_, "poison", failed) || !bind_state(theme, bleed_, "bleed", failed) ||
        !bind_state(theme, emaciated_, "emaciated", failed) || !bind_state(theme, rested_, "rested", failed)) {
        return false;
    }
    theme_ = &theme;
    return true;
}

bool SurvivalRailStates::bind_state(const UiTheme& theme, State& state, const std::string& name,
                                    std::string& failed) {
    // SIGNS[name] = {g, dom, alert} of the mockup: the colour its domain names, whether it alerts, GL[g] in parts.
    const std::string head = "hud_rail_state_" + name + "_";
    ::godot::String domain;
    if (!theme.token_text(head + "domain", domain)) {
        failed = head + "domain";
        return false;
    }
    if (!theme.color(domain.utf8().get_data(), state.tone)) {
        failed = head + "domain: the colour " + std::string(domain.utf8().get_data());
        return false;
    }
    double alert = 0.0;
    if (!read_token(theme, head + "alert", alert, failed)) {
        return false;
    }
    if (!(alert == 0.0 || alert == 1.0)) {
        failed = head + "alert: 0 or 1";
        return false;
    }
    state.alert = alert == 1.0;
    if (!read_part(theme, head + "line", state.line, failed) || !read_part(theme, head + "mark", state.mark, failed) ||
        !read_token(theme, head + "mark_line", state.mark_line, failed) ||
        !read_part(theme, head + "fill", state.fill, failed) ||
        !read_token(theme, head + "fill_opacity", state.fill_opacity, failed) ||
        !read_token(theme, head + "fill_line", state.fill_line, failed)) {
        return false;
    }
    if (!(state.fill_opacity >= 0.0 && state.fill_opacity <= 1.0)) {
        failed = head + "fill_opacity: a share 0..1";
        return false;
    }
    return true;
}

bool SurvivalRailStates::set(const SurvivalBodyFrame& body, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.survival.states: set before bind";
        return false;
    }
    // st.sign.map(k => SIGNS[k]): the record names the marks ase-survival set on the figure this frame.
    poison_.shown = body.poisoned;
    bleed_.shown = body.bleeding;
    emaciated_.shown = body.emaciated;
    rested_.shown = body.well_rested;
    return true;
}

void SurvivalRailStates::stroke(UiPen& canvas, const ::godot::Transform2D& frame,
                                const std::vector<UiSvgOutline>& outlines, const ::godot::Vector2& centre, double size,
                                const ::godot::Color& color, double width) const {
    // The mockup scales the points by k about (x, y) and keeps the stroke width; an open stroke is an L() line,
    // stroke-linecap round.
    for (const UiSvgOutline& outline : outlines) {
        svg_stroke_round(canvas, frame, placed(outline, centre, size), color, width);
    }
}

void SurvivalRailStates::fill(UiPen& canvas, const std::vector<UiSvgOutline>& outlines, const ::godot::Vector2& centre,
                              double size, const ::godot::Color& color) const {
    for (const UiSvgOutline& outline : outlines) {
        svg_fill(canvas, placed(outline, centre, size), color);
    }
}

void SurvivalRailStates::draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still) const {
    if (theme_ == nullptr) {
        return;
    }
    canvas.draw_set_transform_matrix(frame);
    // al = list.filter(x => x.alert).length; rect(x, alert_y, w, base + al * step) in --alert, outside the fade.
    double alerts = 0.0;
    for (const State* walked : {&poison_, &bleed_, &emaciated_, &rested_}) {
        alerts += walked->shown && walked->alert ? 1.0 : 0.0;
    }
    if (alerts > 0.0) {
        canvas.draw_rect(::godot::Rect2(static_cast<float>(alert_x_), static_cast<float>(alert_y_),
                                        static_cast<float>(alert_w_),
                                        static_cast<float>(alert_base_ + alerts * alert_step_)),
                         alert_);
    }
    // list.forEach((g, i) => <g class="fade"> C(x, y, r, {col: c, fill: c}), the pulse, GL[g.g](glyph_x, y, k, c)).
    const double share = fade_.progress(elapsed, 0.0, still);
    double place = 0.0;
    for (const State* walked : {&poison_, &bleed_, &emaciated_, &rested_}) {
        const State& state = *walked;
        if (!state.shown) {
            continue;
        }
        const double y = y_ + place * step_;
        const ::godot::Color tone = faded(state.tone, share);
        const ::godot::Vector2 dot(static_cast<float>(x_), static_cast<float>(y));
        ring_.disc(canvas, dot, radius_, tone);
        ring_.circle(canvas, dot, radius_, tone, dot_line_, 1.0);
        if (state.alert) {
            ring_.pulse(canvas, dot, pulse_radius_, tone, pulse_line_, elapsed, still);
        }
        const ::godot::Vector2 glyph(static_cast<float>(glyph_x_), static_cast<float>(y));
        stroke(canvas, frame, state.line, glyph, glyph_size_, tone, glyph_line_);
        stroke(canvas, frame, state.mark, glyph, glyph_size_, tone, state.mark_line);
        fill(canvas, state.fill, glyph, glyph_size_, faded(tone, state.fill_opacity));
        if (state.fill_line > 0.0) {
            stroke(canvas, frame, state.fill, glyph, glyph_size_, tone, state.fill_line);
        }
        place += 1.0;
    }
}

}  // namespace ase::adp::godot
