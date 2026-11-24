/**
 * ASE GODOT ADAPTER - HUD SURVIVAL REST CARD
 *
 * @file        godot_hud_survival_rest_card.cpp
 * @brief       Draws the rest card SENTADO or DORMINDO on a leader line from the figure as the survival mockup's rest(st)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U05, see godot_hud_survival_rest_card.hpp.
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

#include <ase/adp/godot/godot_hud_survival_rest_card.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>

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

/** Reads the word texts.hud.<key> into `block`; false with the key in `failed`. */
bool read_word(const UiTheme& theme, const char* key, UiTextBlock& block, std::string& failed) {
    std::vector<HudRun> runs;
    if (!theme.words(key, runs)) {
        failed = key;
        return false;
    }
    return block.set(runs, failed);
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

bool SurvivalRestCard::bind(const UiTheme& theme, std::string& failed) {
    if (!line_.bind(theme, failed) || !fade_.bind(theme, "hud_anim_fade", failed) || !sit_.bind_label(theme, failed) ||
        !sleep_.bind_label(theme, failed) || !snore_.bind_label(theme, failed) || !fatigue_.bind_label(theme, failed) ||
        !warmth_.bind_label(theme, failed)) {
        return false;
    }
    if (!read_color(theme, "OVERLAY_INK_SOFT", soft_, failed) || !read_color(theme, "DOMAIN_SKY", sky_, failed) ||
        !read_color(theme, "DOMAIN_CUT", cut_, failed)) {
        return false;
    }
    if (!read_token(theme, "hud_survival_rest_anchor_x", anchor_x_, failed) ||
        !read_token(theme, "hud_survival_rest_anchor_y", anchor_y_, failed) ||
        !read_token(theme, "hud_survival_rest_elbow_x", elbow_x_, failed) ||
        !read_token(theme, "hud_survival_rest_elbow_y", elbow_y_, failed) ||
        !read_token(theme, "hud_survival_rest_column_x", column_x_, failed) ||
        !read_token(theme, "hud_survival_rest_title_dy", title_dy_, failed) ||
        !read_token(theme, "hud_survival_rest_title_size", title_size_, failed) ||
        !read_token(theme, "hud_survival_rest_pose_dx", pose_dx_, failed) ||
        !read_token(theme, "hud_survival_rest_pose_dy", pose_dy_, failed) ||
        !read_token(theme, "hud_survival_rest_pose_size", pose_size_, failed) ||
        !read_token(theme, "hud_survival_rest_pose_line", pose_line_, failed) ||
        !read_token(theme, "hud_survival_rest_arrow_dx", arrow_dx_, failed) ||
        !read_token(theme, "hud_survival_rest_arrow_line", arrow_line_, failed) ||
        !read_token(theme, "hud_survival_rest_head_dx", head_dx_, failed) ||
        !read_token(theme, "hud_survival_rest_head_dy", head_dy_, failed) ||
        !read_token(theme, "hud_survival_rest_fatigue_tip_dy", fatigue_tip_dy_, failed) ||
        !read_token(theme, "hud_survival_rest_fatigue_tail_dy", fatigue_tail_dy_, failed) ||
        !read_token(theme, "hud_survival_rest_fatigue_word_dy", fatigue_word_dy_, failed) ||
        !read_token(theme, "hud_survival_rest_warmth_tip_dy", warmth_tip_dy_, failed) ||
        !read_token(theme, "hud_survival_rest_warmth_tail_dy", warmth_tail_dy_, failed) ||
        !read_token(theme, "hud_survival_rest_warmth_word_dy", warmth_word_dy_, failed) ||
        !read_token(theme, "hud_survival_rest_word_dx", word_dx_, failed) ||
        !read_token(theme, "hud_survival_rest_word_size", word_size_, failed) ||
        !read_token(theme, "hud_survival_rest_snore_near_dx", snore_near_dx_, failed) ||
        !read_token(theme, "hud_survival_rest_snore_near_dy", snore_near_dy_, failed) ||
        !read_token(theme, "hud_survival_rest_snore_near_size", snore_near_size_, failed) ||
        !read_token(theme, "hud_survival_rest_snore_far_dx", snore_far_dx_, failed) ||
        !read_token(theme, "hud_survival_rest_snore_far_dy", snore_far_dy_, failed) ||
        !read_token(theme, "hud_survival_rest_snore_far_size", snore_far_size_, failed)) {
        return false;
    }
    if (!read_glyph(theme, "hud_survival_rest_glyph_sit", sit_glyph_, failed) ||
        !read_glyph(theme, "hud_survival_rest_glyph_sleep", sleep_glyph_, failed)) {
        return false;
    }
    if (!read_word(theme, "survival.rest.sit", sit_, failed) || !read_word(theme, "survival.rest.sleep", sleep_, failed) ||
        !read_word(theme, "survival.rest.snore", snore_, failed) ||
        !read_word(theme, "survival.rest.fatigue", fatigue_, failed) ||
        !read_word(theme, "survival.rest.warmth", warmth_, failed)) {
        return false;
    }
    theme_ = &theme;
    return true;
}

bool SurvivalRestCard::set(const SurvivalRestFrame& rest, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.survival_rest.unbound";
        return false;
    }
    // if (!st.pose) return '': the card stands while the figure rests.
    shown_ = rest.shown;
    sleeping_ = rest.sleeping;
    return true;
}

void SurvivalRestCard::stroke_glyph(UiPen& canvas, const ::godot::Transform2D& frame,
                                    const std::vector<UiSvgOutline>& outlines, const ::godot::Vector2& centre,
                                    double size, const ::godot::Color& color, double width) const {
    // GL[pose](x, y, k, c): the path data is written for k = 1 about the origin; the mockup scales it by k about
    // (x, y) and keeps the stroke width, so the points move and the width stays.
    const float scale = static_cast<float>(size);
    for (const UiSvgOutline& outline : outlines) {
        UiSvgOutline placed;
        placed.closed = outline.closed;
        for (int64_t index = 0; index < outline.points.size(); ++index) {
            placed.points.push_back(centre + outline.points[index] * scale);
        }
        // An open stroke is an L() line: stroke-linecap round.
        svg_stroke_round(canvas, frame, placed, color, width);
    }
}

void SurvivalRestCard::draw_arrow(UiPen& canvas, const ::godot::Transform2D& frame, double x, double tail, double tip,
                                  const ::godot::Color& color) const {
    // L([[x, tail], [x, tip]], {w: 1}) + L([[x - 3, tip + 3], [x, tip], [x + 3, tip + 3]], {w: 1}): the head's knee at
    // the tip is round, as stroke-linejoin round lays it.
    ::godot::PackedVector2Array shaft;
    shaft.push_back(at(x, tail));
    shaft.push_back(at(x, tip));
    svg_polyline_round(canvas, frame, shaft, color, arrow_line_);
    ::godot::PackedVector2Array head;
    head.push_back(at(x - head_dx_, tip + head_dy_));
    head.push_back(at(x, tip));
    head.push_back(at(x + head_dx_, tip + head_dy_));
    svg_polyline_round(canvas, frame, head, color, arrow_line_);
}

void SurvivalRestCard::draw_snore(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& place,
                                  double size, const ::godot::Color& color, double elapsed, bool still) {
    // T(x, y, 'z', {z: size, col: c})
    UiTextPlace letter;
    letter.at = place;
    letter.size = size;
    letter.anchor = GODOT_TEXT_ANCHOR_START;
    letter.bare = true;  // s.html T writes class o.c without sh: no outline
    letter.color = color;
    letter.opacity = 1.0;
    letter.fade = true;
    letter.elapsed = elapsed;
    letter.still = still;
    snore_.draw(canvas, frame, letter);
    canvas.draw_set_transform_matrix(frame);
}

void SurvivalRestCard::draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still) {
    if (theme_ == nullptr || !shown_) {
        return;
    }
    const double share = fade_.progress(elapsed, 0.0, still);
    const double bx = column_x_;
    const double ey = elbow_y_;
    canvas.draw_set_transform_matrix(frame);

    // L([[ax, ay], [ex, ey], [bx, ey]], {col: DOM.sky})
    ::godot::PackedVector2Array leader;
    leader.push_back(at(anchor_x_, anchor_y_));
    leader.push_back(at(elbow_x_, ey));
    leader.push_back(at(bx, ey));
    svg_polyline_round(canvas, frame, leader, faded(sky_, share), line_.width());

    // T(bx, ey-8, st.pose === 'sleep' ? 'DORMINDO' : 'SENTADO', {z: 10, col: DOM.sky})
    UiTextPlace title;
    title.at = at(bx, ey + title_dy_);
    title.size = title_size_;
    title.anchor = GODOT_TEXT_ANCHOR_START;
    title.bare = true;  // s.html T writes class o.c without sh: no outline
    title.color = sky_;
    title.opacity = 1.0;
    title.fade = true;
    title.elapsed = elapsed;
    title.still = still;
    (sleeping_ ? sleep_ : sit_).draw(canvas, frame, title);
    canvas.draw_set_transform_matrix(frame);

    // GL[st.pose](bx+8, ey+16, 8, DOM.sky): sleep carries T(x+k*.2, y-k*.3, 'z', k*.9) and T(x+k*.7, y-k*.85, 'z', k*.7)
    const double px = bx + pose_dx_;
    const double py = ey + pose_dy_;
    const double k = pose_size_;
    stroke_glyph(canvas, frame, sleeping_ ? sleep_glyph_ : sit_glyph_, at(px, py), k, faded(sky_, share), pose_line_);
    if (sleeping_) {
        draw_snore(canvas, frame, at(px + k * snore_near_dx_, py + k * snore_near_dy_), k * snore_near_size_, sky_,
                   elapsed, still);
        draw_snore(canvas, frame, at(px + k * snore_far_dx_, py + k * snore_far_dy_), k * snore_far_size_, sky_,
                   elapsed, still);
    }

    // The arrows of recovery: CANSAÇO in DOM.sky (ey+22 up to ey+8), CALOR in DOM.cut (ey+38 up to ey+24)
    const double ax = bx + arrow_dx_;
    draw_arrow(canvas, frame, ax, ey + fatigue_tail_dy_, ey + fatigue_tip_dy_, faded(sky_, share));
    draw_arrow(canvas, frame, ax, ey + warmth_tail_dy_, ey + warmth_tip_dy_, faded(cut_, share));

    // T(bx+38, ey+15, 'CANSAÇO', {z: 6.4, col: --hud2}), T(bx+38, ey+31, 'CALOR', {z: 6.4, col: --hud2})
    UiTextPlace word;
    word.size = word_size_;
    word.anchor = GODOT_TEXT_ANCHOR_START;
    word.bare = true;  // s.html T writes class o.c without sh: no outline
    word.color = soft_;
    word.opacity = 1.0;
    word.fade = true;
    word.elapsed = elapsed;
    word.still = still;
    word.at = at(bx + word_dx_, ey + fatigue_word_dy_);
    fatigue_.draw(canvas, frame, word);
    word.at = at(bx + word_dx_, ey + warmth_word_dy_);
    warmth_.draw(canvas, frame, word);
}

}  // namespace ase::adp::godot
