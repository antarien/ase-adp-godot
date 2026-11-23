/**
 * ASE GODOT ADAPTER - HUD SURVIVAL AIR MARK
 *
 * @file        godot_hud_survival_air_mark.cpp
 * @brief       Draws the air mark AR TÓXICO in the sky band as the survival mockup's air(st)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U06, see godot_hud_survival_air_mark.hpp.
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

#include <ase/adp/godot/godot_hud_survival_air_mark.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <godot_cpp/variant/vector2.hpp>

#include <cmath>

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

/** Reads the path data of the token `name` into `outlines`; false with the reason in `failed`. */
bool read_glyph(const UiTheme& theme, const char* name, std::vector<UiSvgOutline>& outlines, std::string& failed) {
    ::godot::String text;
    if (!theme.token_text(name, text) || text.is_empty()) {
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

}  // namespace

bool SurvivalAirMark::bind(const UiTheme& theme, std::string& failed) {
    if (!fade_.bind(theme, "hud_anim_fade", failed) || !word_.bind_label(theme, failed)) {
        return false;
    }
    if (!theme.color("DOMAIN_ALERT", alert_)) {
        failed = "DOMAIN_ALERT";
        return false;
    }
    if (!read_token(theme, "hud_survival_air_glyph_x", glyph_x_, failed) ||
        !read_token(theme, "hud_survival_air_glyph_y", glyph_y_, failed) ||
        !read_token(theme, "hud_survival_air_glyph_size", glyph_size_, failed) ||
        !read_token(theme, "hud_survival_air_glyph_line", glyph_line_, failed) ||
        !read_token(theme, "hud_survival_air_word_x", word_x_, failed) ||
        !read_token(theme, "hud_survival_air_word_y", word_y_, failed) ||
        !read_token(theme, "hud_survival_air_word_size", word_size_, failed)) {
        return false;
    }
    if (!read_glyph(theme, "hud_survival_fire_glyph_smoke", glyph_, failed)) {
        return false;
    }
    std::vector<HudRun> runs;
    if (!theme.words("survival.air.toxic", runs)) {
        failed = "survival.air.toxic";
        return false;
    }
    if (!word_.set(runs, failed)) {
        return false;
    }
    theme_ = &theme;
    return true;
}

bool SurvivalAirMark::set(const SurvivalAirFrame& air, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.survival_air.unbound";
        return false;
    }
    // st.air ? `<g class="fade">…</g>` : '': the mark stands while the air is worse than normal.
    toxic_ = air.toxic;
    // The facet Rauch of PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP (symbol Rauchfahne, D2500): the smoke glyph of this mark
    // also stands while soot hangs in the air of the seat's own cell; the boundary is the writer's zero (no fire smoked
    // into the cell), never a threshold of the adapter. The word AR TÓXICO stays the owner's grade of the air alone.
    if (std::isnan(air.soot) || air.soot < 0.0) {
        failed = "hud.survival_air.soot";
        return false;
    }
    smoky_ = air.soot > 0.0;
    return true;
}

void SurvivalAirMark::draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still) {
    if (theme_ == nullptr || !(toxic_ || smoky_)) {
        return;
    }
    const double share = fade_.progress(elapsed, 0.0, still);
    canvas.draw_set_transform_matrix(frame);

    // GL.smoke(276, 64, 5, DOM.alert): the path data is written for k = 1 about the origin; the mockup scales it by
    // k about (x, y) and keeps the stroke width 1, so the points move and the width stays; stroke-linecap round.
    const ::godot::Vector2 centre = at(glyph_x_, glyph_y_);
    const float scale = static_cast<float>(glyph_size_);
    const ::godot::Color ink = faded(alert_, share);
    for (const UiSvgOutline& outline : glyph_) {
        UiSvgOutline placed;
        placed.closed = outline.closed;
        for (int64_t index = 0; index < outline.points.size(); ++index) {
            placed.points.push_back(centre + outline.points[index] * scale);
        }
        svg_stroke_round(canvas, frame, placed, ink, glyph_line_);
    }

    // T(286, 67, 'AR TÓXICO', {z: 6.4, col: DOM.alert}): only while the owner grades the air worse than normal
    if (!toxic_) {
        canvas.draw_set_transform_matrix(frame);
        return;
    }
    UiTextPlace word;
    word.at = at(word_x_, word_y_);
    word.size = word_size_;
    word.anchor = GODOT_TEXT_ANCHOR_START;
    word.bare = true;  // s.html T writes class o.c without sh: no outline
    word.color = alert_;
    word.opacity = 1.0;
    word.fade = true;
    word.elapsed = elapsed;
    word.still = still;
    word_.draw(canvas, frame, word);
    canvas.draw_set_transform_matrix(frame);
}

}  // namespace ase::adp::godot
