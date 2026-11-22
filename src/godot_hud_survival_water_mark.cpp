/**
 * ASE GODOT ADAPTER - HUD SURVIVAL WATER MARK
 *
 * @file        godot_hud_survival_water_mark.cpp
 * @brief       Draws the water mark at the bag as the survival mockup's water(st)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U08, see godot_hud_survival_water_mark.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0528, N0530, carrier N0448
 * @created     2026-11-20
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_survival_water_mark.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <godot_cpp/variant/vector2.hpp>

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

/** Reads the path data of the token `name` into `outlines`; false with the reason in `failed`. */
bool read_glyph(const UiTheme& theme, const char* name, std::vector<UiSvgOutline>& outlines, std::string& failed) {
    ::godot::String text;
    if (!theme.token_text(name, text)) {
        failed = name;
        return false;
    }
    if (!read_svg_path(text.utf8().get_data(), outlines, failed)) {
        failed = std::string(name) + ": " + failed;
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

/** The point (x, y) in mockup units. */
::godot::Vector2 at(double x, double y) {
    return ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
}

/**
 * Strokes `outlines`, written for k = 1 about the origin, scaled by `size` about `centre` at `width` in `ink`, laid in
 * the units of `frame`.
 */
void stroke_placed(UiPen& canvas, const ::godot::Transform2D& frame, const std::vector<UiSvgOutline>& outlines,
                   const ::godot::Vector2& centre, double size, const ::godot::Color& ink, double width) {
    const float scale = static_cast<float>(size);
    for (const UiSvgOutline& outline : outlines) {
        UiSvgOutline placed;
        placed.closed = outline.closed;
        for (int64_t index = 0; index < outline.points.size(); ++index) {
            placed.points.push_back(centre + outline.points[index] * scale);
        }
        // The poison cross is two L() lines, stroke-linecap round; the closed drop has no end.
        svg_stroke_round(canvas, frame, placed, ink, width);
    }
}

}  // namespace

bool SurvivalWaterMark::bind(const UiTheme& theme, std::string& failed) {
    if (!fade_.bind(theme, "hud_anim_fade", failed) || !word_.bind_label(theme, failed)) {
        return false;
    }
    if (!read_color(theme, "OVERLAY_INK_FAINT", faint_, failed) || !read_color(theme, "DOMAIN_ALERT", alert_, failed)) {
        return false;
    }
    if (!read_token(theme, "hud_survival_water_x", x_, failed) ||
        !read_token(theme, "hud_survival_water_y", y_, failed) ||
        !read_token(theme, "hud_survival_water_drop_size", drop_size_, failed) ||
        !read_token(theme, "hud_survival_water_poison_size", poison_size_, failed) ||
        !read_token(theme, "hud_survival_water_word_x", word_x_, failed) ||
        !read_token(theme, "hud_survival_water_word_y", word_y_, failed) ||
        !read_token(theme, "hud_survival_water_word_size", word_size_, failed) ||
        !read_token(theme, "hud_survival_glyph_line", drop_line_, failed) ||
        !read_token(theme, "hud_rail_state_poison_mark_line", cross_line_, failed)) {
        return false;
    }
    if (!read_glyph(theme, "hud_survival_glyph_thirst", drop_, failed) ||
        !read_glyph(theme, "hud_rail_state_poison_mark", cross_, failed)) {
        return false;
    }
    std::vector<HudRun> runs;
    if (!theme.words("survival.water.murky", runs)) {
        failed = "survival.water.murky";
        return false;
    }
    if (!word_.set(runs, failed)) {
        return false;
    }
    theme_ = &theme;
    return true;
}

bool SurvivalWaterMark::set(const SurvivalBodyFrame& body, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.survival_water.unbound";
        return false;
    }
    // st.sign.includes('poison') ? the poison group : the calm drop.
    poisoned_ = body.poisoned;
    return true;
}

void SurvivalWaterMark::draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still) {
    if (theme_ == nullptr) {
        return;
    }
    canvas.draw_set_transform_matrix(frame);
    const ::godot::Vector2 centre = at(x_, y_);
    if (!poisoned_) {
        // GL.thirst(110, 326, 4.5, 'var(--hud3)'): outside any fade group.
        stroke_placed(canvas, frame, drop_, centre, drop_size_, faint_, drop_line_);
        return;
    }

    // <g class="fade">GL.poison(110, 326, 5, DOM.alert) T(120, 322, 'ÁGUA TURVA', {z: 6.4, col: DOM.alert})</g>
    const double share = fade_.progress(elapsed, 0.0, still);
    const ::godot::Color ink = faded(alert_, share);
    stroke_placed(canvas, frame, drop_, centre, poison_size_, ink, drop_line_);
    stroke_placed(canvas, frame, cross_, centre, poison_size_, ink, cross_line_);

    UiTextPlace word;
    word.at = at(word_x_, word_y_);
    word.size = word_size_;
    word.anchor = GODOT_TEXT_ANCHOR_START;
    word.color = alert_;
    word.opacity = 1.0;
    word.fade = true;
    word.elapsed = elapsed;
    word.still = still;
    word_.draw(canvas, frame, word);
    canvas.draw_set_transform_matrix(frame);
}

}  // namespace ase::adp::godot
