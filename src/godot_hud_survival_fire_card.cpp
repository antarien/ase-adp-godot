/**
 * ASE GODOT ADAPTER - HUD SURVIVAL FIRE CARD
 *
 * @file        godot_hud_survival_fire_card.cpp
 * @brief       Draws the fire card FOGUEIRA on a leader line from the camp fire as the survival mockup's fireCard(st)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U04, see godot_hud_survival_fire_card.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0528, B0657, carrier N0448
 * @created     2026-11-20
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_survival_fire_card.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/hud/types.hpp>

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

/** The rounded rectangle (x, y, w, h) with corner `round` as one closed outline: the mockup's <rect rx>. */
UiSvgOutline rounded(double x, double y, double w, double h, double round) {
    UiSvgOutline outline;
    svg_append_rounded_rect(outline,
                            ::godot::Rect2(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w),
                                           static_cast<float>(h)),
                            static_cast<float>(round));
    return outline;
}

}  // namespace

bool SurvivalFireCard::bind(const UiTheme& theme, std::string& failed) {
    if (!line_.bind(theme, failed) || !fade_.bind(theme, "hud_anim_fade", failed) ||
        !title_.bind_label(theme, failed) || !fuel_.bind_data(theme, failed) || !air_.bind_label(theme, failed) ||
        !smoke_.bind_label(theme, failed) || !ash_word_.bind_label(theme, failed) ||
        !ash_kg_.bind_data(theme, failed) || !burn_.bind_data(theme, failed)) {
        return false;
    }
    if (!read_color(theme, "OVERLAY_INK_FAINT", faint_, failed) || !read_color(theme, "OVERLAY_INK_SOFT", soft_, failed) ||
        !read_color(theme, "DOMAIN_CUT", cut_, failed) || !read_color(theme, "DOMAIN_GROUND", ground_, failed) ||
        !read_color(theme, "DOMAIN_FLOW", flow_, failed) || !read_color(theme, "DOMAIN_ALERT", alert_, failed)) {
        return false;
    }
    if (!read_token(theme, "hud_survival_fire_anchor_x", anchor_x_, failed) ||
        !read_token(theme, "hud_survival_fire_anchor_y", anchor_y_, failed) ||
        !read_token(theme, "hud_survival_fire_elbow_x", elbow_x_, failed) ||
        !read_token(theme, "hud_survival_fire_elbow_y", elbow_y_, failed) ||
        !read_token(theme, "hud_survival_fire_column_x", column_x_, failed) ||
        !read_token(theme, "hud_survival_fire_title_dy", title_dy_, failed) ||
        !read_token(theme, "hud_survival_fire_title_size", title_size_, failed) ||
        !read_token(theme, "hud_survival_fire_stage_dx", stage_dx_, failed) ||
        !read_token(theme, "hud_survival_fire_stage_step", stage_step_, failed) ||
        !read_token(theme, "hud_survival_fire_stage_dy", stage_dy_, failed) ||
        !read_token(theme, "hud_survival_fire_stage_size", stage_size_, failed) ||
        !read_token(theme, "hud_survival_fire_ring_radius", ring_radius_, failed) ||
        !read_token(theme, "hud_survival_fire_ring_line", ring_line_, failed) ||
        !read_token(theme, "hud_survival_fire_log_dy", log_dy_, failed) ||
        !read_token(theme, "hud_survival_fire_log_top", log_top_, failed) ||
        !read_token(theme, "hud_survival_fire_log_w", log_w_, failed) ||
        !read_token(theme, "hud_survival_fire_log_h", log_h_, failed) ||
        !read_token(theme, "hud_survival_fire_log_round", log_round_, failed) ||
        !read_token(theme, "hud_survival_fire_log_line", log_line_, failed) ||
        !read_token(theme, "hud_survival_fire_fill_inset", fill_inset_, failed) ||
        !read_token(theme, "hud_survival_fire_fill_w", fill_w_, failed) ||
        !read_token(theme, "hud_survival_fire_fill_h", fill_h_, failed) ||
        !read_token(theme, "hud_survival_fire_fill_round", fill_round_, failed) ||
        !read_token(theme, "hud_survival_fire_fuel_dx", fuel_dx_, failed) ||
        !read_token(theme, "hud_survival_fire_fuel_dy", fuel_dy_, failed) ||
        !read_token(theme, "hud_survival_fire_fuel_size", fuel_size_, failed) ||
        !read_token(theme, "hud_survival_fire_row_glyph_dy", row_glyph_dy_, failed) ||
        !read_token(theme, "hud_survival_fire_row_word_dy", row_word_dy_, failed) ||
        !read_token(theme, "hud_survival_fire_row_word_size", row_word_size_, failed) ||
        !read_token(theme, "hud_survival_fire_air_dx", air_dx_, failed) ||
        !read_token(theme, "hud_survival_fire_air_size", air_size_, failed) ||
        !read_token(theme, "hud_survival_fire_air_word_dx", air_word_dx_, failed) ||
        !read_token(theme, "hud_survival_fire_smoke_dx", smoke_dx_, failed) ||
        !read_token(theme, "hud_survival_fire_smoke_size", smoke_size_, failed) ||
        !read_token(theme, "hud_survival_fire_smoke_word_dx", smoke_word_dx_, failed) ||
        !read_token(theme, "hud_survival_fire_ash_dx", ash_dx_, failed) ||
        !read_token(theme, "hud_survival_fire_ash_dy", ash_dy_, failed) ||
        !read_token(theme, "hud_survival_fire_ash_size", ash_size_, failed) ||
        !read_token(theme, "hud_survival_fire_ash_word_dx", ash_word_dx_, failed) ||
        !read_token(theme, "hud_survival_fire_ash_kg_dx", ash_kg_dx_, failed) ||
        !read_token(theme, "hud_survival_fire_ash_kg_decimals", ash_kg_decimals_, failed) ||
        !read_token(theme, "hud_survival_fire_ash_empty", ash_empty_, failed) ||
        !read_token(theme, "hud_survival_fire_burn_dx", burn_dx_, failed) ||
        !read_token(theme, "hud_survival_fire_burn_dy", burn_dy_, failed) ||
        !read_token(theme, "hud_survival_fire_burn_unit_s", burn_unit_s_, failed) ||
        !read_token(theme, "hud_survival_fire_burn_decimals", burn_decimals_, failed) ||
        !read_token(theme, "hud_survival_glyph_line", glyph_line_, failed) ||
        !read_token(theme, "hud_survival_fire_glyph_air_line", air_line_, failed) ||
        !read_token(theme, "hud_survival_fire_glyph_dot_line", dot_line_, failed) ||
        !read_token(theme, "hud_survival_fire_glyph_ember_dot_radius", ember_dot_, failed) ||
        !read_token(theme, "hud_survival_fire_glyph_ash_dot_radius", ash_dot_, failed)) {
        return false;
    }
    // STG = [['ignite','ACENDER'], ['flame','CHAMA'], ['charcoal','CARVÃO'], ['ember','BRASA'], ['ash','CINZA']]
    stages_.assign(STAGES, Stage());
    if (!bind_stage(theme, stages_[0], "hud_survival_fire_glyph_ignite", "hud_survival_fire_glyph_ignite_line",
                    "survival.fire.stage.0", failed) ||
        !bind_stage(theme, stages_[1], "hud_survival_fire_glyph_flame", nullptr, "survival.fire.stage.1", failed) ||
        !bind_stage(theme, stages_[2], "hud_survival_fire_glyph_charcoal", nullptr, "survival.fire.stage.2", failed) ||
        !bind_stage(theme, stages_[3], "hud_survival_fire_glyph_ember", nullptr, "survival.fire.stage.3", failed) ||
        !bind_stage(theme, stages_[4], "hud_survival_fire_glyph_ash", nullptr, "survival.fire.stage.4", failed)) {
        return false;
    }
    double mark_line = 0.0;
    if (!read_glyph(theme, "hud_survival_fire_glyph_charcoal_mark", stages_[2].mark, failed) ||
        !read_token(theme, "hud_survival_fire_glyph_charcoal_mark_line", mark_line, failed) ||
        !read_glyph(theme, "hud_survival_fire_glyph_ember_dots", stages_[3].dots, failed) ||
        !read_glyph(theme, "hud_survival_fire_glyph_ash_dots", stages_[4].dots, failed) ||
        !read_glyph(theme, "hud_survival_fire_glyph_air", air_glyph_, failed) ||
        !read_glyph(theme, "hud_survival_fire_glyph_smoke", smoke_glyph_, failed)) {
        return false;
    }
    stages_[2].mark_line = mark_line;
    stages_[3].dot_radius = ember_dot_;
    stages_[4].dot_radius = ash_dot_;
    if (!theme.words("survival.fire.title", title_runs_)) {
        failed = "survival.fire.title";
        return false;
    }
    if (!theme.words("survival.fire.fuel", fuel_runs_)) {
        failed = "survival.fire.fuel";
        return false;
    }
    if (!read_word(theme, "survival.fire.air", air_, failed) || !read_word(theme, "survival.fire.smoke", smoke_, failed) ||
        !read_word(theme, "survival.fire.ash", ash_word_, failed)) {
        return false;
    }
    if (!theme.words("survival.fire.ash_kg", ash_kg_runs_)) {
        failed = "survival.fire.ash_kg";
        return false;
    }
    if (!theme.words("survival.fire.burn", burn_runs_)) {
        failed = "survival.fire.burn";
        return false;
    }
    // The burn time is drawn in a unit of the word (seconds per unit); a unit of none would divide by zero.
    if (!(burn_unit_s_ > 0.0)) {
        failed = "hud_survival_fire_burn_unit_s: above 0";
        return false;
    }
    theme_ = &theme;
    return true;
}

bool SurvivalFireCard::bind_stage(const UiTheme& theme, Stage& stage, const char* glyph, const char* line,
                                  const char* word, std::string& failed) {
    if (!read_glyph(theme, glyph, stage.glyph, failed)) {
        return false;
    }
    stage.line = glyph_line_;
    if (line != nullptr && !read_token(theme, line, stage.line, failed)) {
        return false;
    }
    return plain_word(theme, word, stage.word, failed);
}

bool SurvivalFireCard::set(const SurvivalFireFrame& fire, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.survival_fire.unbound";
        return false;
    }
    // if (!st.fire) return '': the card stands while a camp fire stands by the figure.
    shown_ = fire.shown;
    if (!shown_) {
        return true;
    }
    if (fire.stage < 0 || fire.stage >= static_cast<int32_t>(stages_.size())) {
        failed = "hud.survival_fire.stage: 0 to 4";
        return false;
    }
    stage_ = fire.stage;
    fuel_share_ = fire.fuel < 0.0 ? 0.0 : (fire.fuel > 1.0 ? 1.0 : fire.fuel);
    toxic_ = fire.toxic;
    oxygen_ = fire.oxygen < 0.0 ? 0.0 : (fire.oxygen > 1.0 ? 1.0 : fire.oxygen);
    // 'FOGUEIRA · ' + STG[st.stage][1]
    std::vector<HudRun> title = title_runs_;
    if (!fill_words(title, {HudPortValue{"stage", stages_[static_cast<size_t>(stage_)].word}}, failed) ||
        !title_.set(title, failed)) {
        return false;
    }
    // 'LENHA ' + Math.round(st.fuel * 100) + ' %'
    std::vector<HudRun> fuel = fuel_runs_;
    if (!fill_words(fuel, {HudPortValue{"value", hud_rounded(fuel_share_ * GODOT_PERCENT, 0)}}, failed) ||
        !fuel_.set(fuel, failed)) {
        return false;
    }
    // What the fire leaves (ingle.left.v1 ash_kg): the kg of ash right of CINZA; the heap inks with it.
    ash_kg_value_ = fire.ash_kg < 0.0 ? 0.0 : fire.ash_kg;
    std::vector<HudRun> ash_kg = ash_kg_runs_;
    if (!fill_words(ash_kg,
                    {HudPortValue{"value", hud_rounded(ash_kg_value_, static_cast<int32_t>(ash_kg_decimals_))}},
                    failed) ||
        !ash_kg_.set(ash_kg, failed)) {
        return false;
    }
    // How long the fire lasts (ingle.left.v1 burn_left): the burn time under LENHA, shown while the cell burns at a
    // rate; a cell that heats or is ash has none.
    burning_ = fire.burning;
    if (!burning_) {
        return true;
    }
    const double left = fire.burn_left < 0.0 ? 0.0 : fire.burn_left;
    std::vector<HudRun> burn = burn_runs_;
    if (!fill_words(burn,
                    {HudPortValue{"value",
                                  hud_rounded(left / burn_unit_s_, static_cast<int32_t>(burn_decimals_))}},
                    failed)) {
        return false;
    }
    return burn_.set(burn, failed);
}

void SurvivalFireCard::stroke_glyph(UiPen& canvas, const ::godot::Transform2D& frame,
                                    const std::vector<UiSvgOutline>& outlines, const ::godot::Vector2& centre,
                                    double size, const ::godot::Color& color, double width) const {
    // GL[name](x, y, k, c): the path data is written for k = 1 about the origin; the mockup scales it by k about
    // (x, y) and keeps the stroke width, so the points move and the width stays.
    const float scale = static_cast<float>(size);
    for (const UiSvgOutline& outline : outlines) {
        UiSvgOutline placed;
        placed.closed = outline.closed;
        for (int64_t index = 0; index < outline.points.size(); ++index) {
            placed.points.push_back(centre + outline.points[index] * scale);
        }
        // An open stroke is an L() line or a round-capped path: stroke-linecap round.
        svg_stroke_round(canvas, frame, placed, color, width);
    }
}

void SurvivalFireCard::draw_dots(UiPen& canvas, const std::vector<UiSvgOutline>& outlines,
                                 const ::godot::Vector2& centre, double size, double radius,
                                 const ::godot::Color& color) const {
    // C(x + dx * k, y + dy * k, r, {col: c, fill: c}): the place scales with k, the radius stays in pixels.
    const float scale = static_cast<float>(size);
    for (const UiSvgOutline& outline : outlines) {
        if (outline.points.size() == 0) {
            continue;
        }
        const ::godot::Vector2 place = centre + outline.points[0] * scale;
        canvas.draw_circle(place, static_cast<float>(radius), color, true);
        canvas.draw_circle(place, static_cast<float>(radius), color, false, static_cast<float>(dot_line_));
    }
}

void SurvivalFireCard::draw_stage(UiPen& canvas, const ::godot::Transform2D& frame, const Stage& stage,
                                  const ::godot::Vector2& centre, double size, const ::godot::Color& color) const {
    stroke_glyph(canvas, frame, stage.glyph, centre, size, color, stage.line);
    stroke_glyph(canvas, frame, stage.mark, centre, size, color, stage.mark_line);
    draw_dots(canvas, stage.dots, centre, size, stage.dot_radius, color);
}

void SurvivalFireCard::draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still) {
    if (theme_ == nullptr || !shown_) {
        return;
    }
    const double share = fade_.progress(elapsed, 0.0, still);
    const double bx = column_x_;
    const double ey = elbow_y_;
    canvas.draw_set_transform_matrix(frame);

    // L([[ax, ay], [ex, ey], [bx, ey]], {col: DOM.cut})
    ::godot::PackedVector2Array leader;
    leader.push_back(at(anchor_x_, anchor_y_));
    leader.push_back(at(elbow_x_, ey));
    leader.push_back(at(bx, ey));
    svg_polyline_round(canvas, frame, leader, faded(cut_, share), line_.width());

    // T(bx, ey-8, 'FOGUEIRA · ' + STG[st.stage][1], {z: 10, col: DOM.cut})
    UiTextPlace title;
    title.at = at(bx, ey + title_dy_);
    title.size = title_size_;
    title.anchor = GODOT_TEXT_ANCHOR_START;
    title.color = cut_;
    title.opacity = 1.0;
    title.fade = true;
    title.elapsed = elapsed;
    title.still = still;
    title_.draw(canvas, frame, title);
    canvas.draw_set_transform_matrix(frame);

    // STG.forEach((g, i) => GL[g[0]](bx+8+i*24, ey+16, 5.5, on ? DOM.cut : --hud3); on: C(x, ey+16, 9, {w: .6}))
    for (size_t index = 0; index < stages_.size(); ++index) {
        const bool on = static_cast<int32_t>(index) == stage_;
        const ::godot::Color tone = faded(on ? cut_ : faint_, share);
        const ::godot::Vector2 centre = at(bx + stage_dx_ + static_cast<double>(index) * stage_step_, ey + stage_dy_);
        draw_stage(canvas, frame, stages_[index], centre, stage_size_, tone);
        if (on) {
            canvas.draw_circle(centre, static_cast<float>(ring_radius_), tone, false, static_cast<float>(ring_line_));
        }
    }

    // <rect x=bx y=ly-4 width=70 height=7 rx=3.5 stroke --hud2 .8> and <rect x=bx+1.5 y=ly-2.5 width=67*fuel
    // height=4 rx=2 fill DOM.ground>
    const double ly = ey + log_dy_;
    svg_stroke(canvas, rounded(bx, ly + log_top_, log_w_, log_h_, log_round_), faded(soft_, share), log_line_);
    if (fuel_share_ > 0.0) {
        svg_fill(canvas,
                 rounded(bx + fill_inset_, ly + log_top_ + fill_inset_, fill_w_ * fuel_share_, fill_h_, fill_round_),
                 faded(ground_, share));
    }

    // T(bx+76, ly+2.5, 'LENHA n %', {c: d, z: 6.2, col: --hud2})
    UiTextPlace fuel;
    fuel.at = at(bx + fuel_dx_, ly + fuel_dy_);
    fuel.size = fuel_size_;
    fuel.anchor = GODOT_TEXT_ANCHOR_START;
    fuel.color = soft_;
    fuel.opacity = 1.0;
    fuel.fade = true;
    fuel.elapsed = elapsed;
    fuel.still = still;
    fuel_.draw(canvas, frame, fuel);
    canvas.draw_set_transform_matrix(frame);

    // The burn time under LENHA, in the same class and ink, while the cell burns at a rate (ingle.left.v1).
    if (burning_) {
        UiTextPlace burn = fuel;
        burn.at = at(bx + burn_dx_, ly + burn_dy_);
        burn_.draw(canvas, frame, burn);
        canvas.draw_set_transform_matrix(frame);
    }

    // GL.air(bx+6, ly+20, 5, DOM.flow), GL.smoke(bx+52, ly+20, 6, air ? DOM.alert : --hud2), GL.ash(bx+104, ly+22, 6)
    // The air stream carries the oxygen the fire draws (ATM_OXYGEN_FACTOR): full at normal air, gone at none.
    const ::godot::Color smoke_tone = toxic_ ? alert_ : soft_;
    stroke_glyph(canvas, frame, air_glyph_, at(bx + air_dx_, ly + row_glyph_dy_), air_size_,
                 faded(flow_, share * oxygen_), air_line_);
    stroke_glyph(canvas, frame, smoke_glyph_, at(bx + smoke_dx_, ly + row_glyph_dy_), smoke_size_,
                 faded(smoke_tone, share), glyph_line_);
    // The ash heap carries what the fire left (HUB_ENT_ASH_REQ_ASH): at ash_empty while no ash is left, full once.
    const double ash_ink = ash_kg_value_ > 0.0 ? 1.0 : ash_empty_;
    draw_stage(canvas, frame, stages_[STAGE_ASH], at(bx + ash_dx_, ly + ash_dy_), ash_size_,
               faded(soft_, share * ash_ink));

    // T(bx+18, ly+22, 'AR', {z: 6.6, col: DOM.flow}), T(bx+62, .., 'FUMO'), T(bx+114, .., 'CINZA', {col: --hud2})
    UiTextPlace word;
    word.size = row_word_size_;
    word.anchor = GODOT_TEXT_ANCHOR_START;
    word.opacity = 1.0;
    word.fade = true;
    word.elapsed = elapsed;
    word.still = still;
    word.at = at(bx + air_word_dx_, ly + row_word_dy_);
    word.color = flow_;
    air_.draw(canvas, frame, word);
    word.at = at(bx + smoke_word_dx_, ly + row_word_dy_);
    word.color = smoke_tone;
    smoke_.draw(canvas, frame, word);
    word.at = at(bx + ash_word_dx_, ly + row_word_dy_);
    word.color = soft_;
    ash_word_.draw(canvas, frame, word);
    word.at = at(bx + ash_kg_dx_, ly + row_word_dy_);
    ash_kg_.draw(canvas, frame, word);
}

}  // namespace ase::adp::godot
