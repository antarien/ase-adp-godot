/**
 * ASE GODOT ADAPTER - HUD SURVIVAL WARMTH SCALE
 *
 * @file        godot_hud_survival_warmth_scale.cpp
 * @brief       Draws the warmth scale CONFORTO over the heat row of the body rail as the survival mockup's corpo(st)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U03, see godot_hud_survival_warmth_scale.hpp.
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

#include <ase/adp/godot/godot_hud_survival_warmth_scale.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/hud/ring/types.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>
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

bool SurvivalWarmthScale::bind(const UiTheme& theme, std::string& failed) {
    if (!line_.bind(theme, failed) || !fade_.bind(theme, "hud_anim_fade", failed) ||
        !label_.bind_label(theme, failed) || !low_.bind_data(theme, failed) || !high_.bind_data(theme, failed)) {
        return false;
    }
    if (!read_color(theme, "OVERLAY_INK", ink_, failed) || !read_color(theme, "OVERLAY_INK_FAINT", faint_, failed) ||
        !read_color(theme, "OVERLAY_INK_SOFT", soft_, failed) || !read_color(theme, "DOMAIN_CUT", cut_, failed) ||
        !read_color(theme, "DOMAIN_LIFE", life_, failed)) {
        return false;
    }
    if (!read_token(theme, "hud_survival_rail_row_y", row_y_, failed) ||
        !read_token(theme, "hud_survival_rail_comfort_low", comfort_low_, failed) ||
        !read_token(theme, "hud_survival_rail_comfort_high", comfort_high_, failed) ||
        !read_token(theme, "hud_survival_comfort_left", left_, failed) ||
        !read_token(theme, "hud_survival_comfort_right", right_, failed) ||
        !read_token(theme, "hud_survival_comfort_span", span_, failed) ||
        !read_token(theme, "hud_survival_comfort_rise", rise_, failed) ||
        !read_token(theme, "hud_survival_comfort_leader_x", leader_x_, failed) ||
        !read_token(theme, "hud_survival_comfort_leader_bend", leader_bend_, failed) ||
        !read_token(theme, "hud_survival_comfort_leader_opacity", leader_opacity_, failed) ||
        !read_token(theme, "hud_survival_comfort_band_line", band_line_, failed) ||
        !read_token(theme, "hud_survival_comfort_marker_gap", marker_gap_, failed) ||
        !read_token(theme, "hud_survival_comfort_marker_h", marker_h_, failed) ||
        !read_token(theme, "hud_survival_comfort_marker_half", marker_half_, failed) ||
        !read_token(theme, "hud_survival_comfort_tick_dy", tick_dy_, failed) ||
        !read_token(theme, "hud_survival_comfort_tick_size", tick_size_, failed) ||
        !read_token(theme, "hud_survival_comfort_label_dy", label_dy_, failed) ||
        !read_token(theme, "hud_survival_comfort_label_size", label_size_, failed)) {
        return false;
    }
    if (!(span_ > 0.0)) {
        failed = "hud_survival_comfort_span: above 0";
        return false;
    }
    std::vector<HudRun> word;
    if (!theme.words("survival.body.comfort", word)) {
        failed = "survival.body.comfort";
        return false;
    }
    if (!label_.set(word, failed)) {
        return false;
    }
    if (!theme.words("survival.body.bound", bound_)) {
        failed = "survival.body.bound";
        return false;
    }
    theme_ = &theme;
    // T(t2x(18), y-14, '18') and T(t2x(26), y-14, '26'): the band's ends never change within a view.
    if (!set_bound(low_, comfort_low_, failed) || !set_bound(high_, comfort_high_, failed)) {
        theme_ = nullptr;
        return false;
    }
    return true;
}

bool SurvivalWarmthScale::set_bound(UiTextBlock& block, double degrees, std::string& failed) {
    std::vector<HudRun> runs = bound_;
    if (!fill_words(runs, {HudPortValue{"value", hud_rounded(degrees, 0)}}, failed)) {
        return false;
    }
    return block.set(runs, failed);
}

double SurvivalWarmthScale::place_of(double degrees) const {
    // t2x = t => x0 + (t - 0) / 40 * (x1 - x0)
    return left_ + degrees / span_ * (right_ - left_);
}

bool SurvivalWarmthScale::set(const SurvivalBodyFrame& body, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.survival_warmth.unbound";
        return false;
    }
    // if (st.vit.temp): the scale stands while the felt temperature counts for the figure.
    shown_ = body.temp.counts;
    felt_ = body.temp.value;
    return true;
}

void SurvivalWarmthScale::draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still) {
    if (theme_ == nullptr || !shown_) {
        return;
    }
    const double share = fade_.progress(elapsed, 0.0, still);
    const double y = row_y_;
    const double scale_y = y - rise_;
    canvas.draw_set_transform_matrix(frame);

    // L([[343, y], [300, y-22], [x1, y-22]], {col: DOM.cut, op: .7})
    ::godot::PackedVector2Array leader;
    leader.push_back(at(leader_x_, y));
    leader.push_back(at(leader_bend_, scale_y));
    leader.push_back(at(right_, scale_y));
    svg_polyline_round(canvas, frame, leader, faded(cut_, share * leader_opacity_), line_.width());

    // L([[x0, y-22], [x1, y-22]], {col: --hud3})
    ::godot::PackedVector2Array scale;
    scale.push_back(at(left_, scale_y));
    scale.push_back(at(right_, scale_y));
    svg_polyline_round(canvas, frame, scale, faded(faint_, share), line_.width());

    // L([[t2x(18), y-22], [t2x(26), y-22]], {col: DOM.life, w: 2})
    ::godot::PackedVector2Array band;
    band.push_back(at(place_of(comfort_low_), scale_y));
    band.push_back(at(place_of(comfort_high_), scale_y));
    svg_polyline_round(canvas, frame, band, faded(life_, share), band_line_);

    // polygon t2x(v),y-25  t2x(v)-2.4,y-29  t2x(v)+2.4,y-29, fill --hud
    const double marker_x = place_of(felt_);
    const double tip_y = scale_y - marker_gap_;
    ::godot::PackedVector2Array marker;
    marker.push_back(at(marker_x, tip_y));
    marker.push_back(at(marker_x - marker_half_, tip_y - marker_h_));
    marker.push_back(at(marker_x + marker_half_, tip_y - marker_h_));
    canvas.draw_colored_polygon(marker, faded(ink_, share));

    // T(t2x(18), y-14, '18', {c: d, z: 5.6, a: middle, col: --hud3}) and the same for 26
    UiTextPlace tick;
    tick.size = tick_size_;
    tick.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    tick.bare = true;  // s.html T writes class o.c without sh: no outline
    tick.color = faint_;
    tick.opacity = 1.0;
    tick.fade = true;
    tick.elapsed = elapsed;
    tick.still = still;
    tick.at = at(place_of(comfort_low_), scale_y + tick_dy_);
    low_.draw(canvas, frame, tick);
    tick.at = at(place_of(comfort_high_), scale_y + tick_dy_);
    high_.draw(canvas, frame, tick);

    // T(x0, y-30, 'CONFORTO', {z: 6.2, col: --hud2})
    UiTextPlace word;
    word.at = at(left_, scale_y - label_dy_);
    word.size = label_size_;
    word.anchor = GODOT_TEXT_ANCHOR_START;
    word.bare = true;  // s.html T writes class o.c without sh: no outline
    word.color = soft_;
    word.opacity = 1.0;
    word.fade = true;
    word.elapsed = elapsed;
    word.still = still;
    label_.draw(canvas, frame, word);
}

}  // namespace ase::adp::godot
