/**
 * ASE GODOT ADAPTER - HUD SURVIVAL BODY RAIL
 *
 * @file        godot_hud_survival_body_rail.cpp
 * @brief       Draws the body rail CORPO at the right edge of the care view as the survival mockup's corpo(st) does
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U01, see godot_hud_survival_body_rail.hpp.
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

#include <ase/adp/godot/godot_hud_survival_body_rail.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <ase/math/math.hpp>

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <initializer_list>

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

/** `color` with its alpha scaled by `share`: the fade of the group the mark stands in. */
::godot::Color faded(const ::godot::Color& color, double share) {
    ::godot::Color out = color;
    out.a = static_cast<float>(out.a * share);
    return out;
}

}  // namespace

bool SurvivalBodyRail::bind(const UiTheme& theme, std::string& failed) {
    if (!line_.bind(theme, failed) || !fade_.bind(theme, "hud_anim_fade", failed) ||
        !label_.bind_label(theme, failed)) {
        return false;
    }
    if (!read_color(theme, "OVERLAY_INK", ink_, failed) || !read_color(theme, "OVERLAY_INK_FAINT", faint_, failed) ||
        !read_color(theme, "OVERLAY_INK_SOFT", soft_, failed) || !read_color(theme, "DOMAIN_SKY", sky_, failed) ||
        !read_color(theme, "DOMAIN_CUT", cut_, failed) || !read_color(theme, "DOMAIN_LIFE", life_, failed) ||
        !read_color(theme, "DOMAIN_GROUND", ground_, failed) || !read_color(theme, "DOMAIN_WATER", water_, failed) ||
        !read_color(theme, "DOMAIN_TIME", time_, failed) || !read_color(theme, "DOMAIN_ALERT", alert_, failed)) {
        return false;
    }
    if (!read_token(theme, "hud_survival_rail_x", x_, failed) ||
        !read_token(theme, "hud_survival_rail_top", top_, failed) ||
        !read_token(theme, "hud_survival_rail_bottom", bottom_, failed) ||
        !read_token(theme, "hud_survival_rail_label_y", label_y_, failed) ||
        !read_token(theme, "hud_survival_rail_label_size", label_size_, failed) ||
        !read_token(theme, "hud_survival_rail_label_turn", label_turn_, failed) ||
        !read_token(theme, "hud_survival_rail_row_y", row_y_, failed) ||
        !read_token(theme, "hud_survival_rail_row_step", row_step_, failed) ||
        !read_token(theme, "hud_survival_rail_row_delay", row_delay_, failed) ||
        !read_token(theme, "hud_survival_rail_glyph_size", glyph_size_, failed) ||
        !read_token(theme, "hud_survival_rail_track_left", track_left_, failed) ||
        !read_token(theme, "hud_survival_rail_track_right", track_right_, failed) ||
        !read_token(theme, "hud_survival_rail_track_dy", track_dy_, failed) ||
        !read_token(theme, "hud_survival_rail_fill_line", fill_line_, failed) ||
        !read_token(theme, "hud_survival_rail_name_x", name_x_, failed) ||
        !read_token(theme, "hud_survival_rail_name_size", name_size_, failed) ||
        !read_token(theme, "hud_survival_rail_text_dy", text_dy_, failed) ||
        !read_token(theme, "hud_survival_rail_number_x", number_x_, failed) ||
        !read_token(theme, "hud_survival_rail_number_size", number_size_, failed) ||
        !read_token(theme, "hud_survival_rail_pulse_y", pulse_y_, failed) ||
        !read_token(theme, "hud_survival_rail_pulse_size", pulse_size_, failed) ||
        !read_token(theme, "hud_survival_rail_level_full", level_full_, failed) ||
        !read_token(theme, "hud_survival_rail_stress_onset", stress_onset_, failed) ||
        !read_token(theme, "hud_survival_rail_health_alert", health_alert_, failed) ||
        !read_token(theme, "hud_survival_rail_comfort_low", comfort_low_, failed) ||
        !read_token(theme, "hud_survival_rail_temp_decimals", temp_decimals_, failed) ||
        !read_token(theme, "hud_survival_rail_hit_w", hit_w_, failed) ||
        !read_token(theme, "hud_survival_rail_hit_h", hit_h_, failed) ||
        !read_token(theme, "hud_survival_glyph_line", glyph_line_, failed) ||
        !read_token(theme, "hud_survival_glyph_stem_line", stem_line_, failed)) {
        return false;
    }
    std::vector<HudRun> word;
    if (!theme.words("survival.body.label", word)) {
        failed = "survival.body.label";
        return false;
    }
    if (!label_.set(word, failed) || !read_glyph(theme, "hud_survival_glyph_pulse", pulse_, failed)) {
        return false;
    }
    theme_ = &theme;
    // s.html VIT: heat, hunger, thirst, energy, fatigue, health; draw() walks the rows in this order.
    if (!bind_row(heat_, "survival.body.temp", "hud_survival_glyph_temp", "survival.body.degrees", failed) ||
        !read_glyph(theme, "hud_survival_glyph_temp_stem", heat_.stem, failed) ||
        !bind_row(hunger_, "survival.body.hunger", "hud_survival_glyph_hunger", "survival.body.percent", failed) ||
        !bind_row(thirst_, "survival.body.thirst", "hud_survival_glyph_thirst", "survival.body.percent", failed) ||
        !bind_row(energy_, "survival.body.energy", "hud_survival_glyph_energy", "survival.body.of_full", failed) ||
        !bind_row(fatigue_, "survival.body.fatigue", "hud_survival_glyph_fatigue", "survival.body.percent", failed) ||
        !bind_row(health_, "survival.body.health", "hud_survival_glyph_health", "survival.body.percent", failed)) {
        theme_ = nullptr;
        return false;
    }
    return true;
}

bool SurvivalBodyRail::bind_row(Row& row, const char* word, const char* glyph, const char* format,
                                std::string& failed) {
    if (!row.name.bind_label(*theme_, failed) || !row.number.bind_data(*theme_, failed)) {
        return false;
    }
    std::vector<HudRun> runs;
    if (!theme_->words(word, runs)) {
        failed = word;
        return false;
    }
    if (!row.name.set(runs, failed) || !read_glyph(*theme_, glyph, row.glyph, failed)) {
        return false;
    }
    const std::string joined = std::string(glyph) + "_joined";
    if (!read_token(*theme_, joined.c_str(), row.joined, failed)) {
        return false;
    }
    if (!theme_->words(format, row.format)) {
        failed = format;
        return false;
    }
    return true;
}

bool SurvivalBodyRail::set_number(Row& row, const std::string& text, std::string& failed) {
    std::vector<HudRun> runs = row.format;
    if (!fill_words(runs, {HudPortValue{"value", text}}, failed)) {
        return false;
    }
    return row.number.set(runs, failed);
}

bool SurvivalBodyRail::set(const SurvivalBodyFrame& body, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "hud.survival_body.unbound";
        return false;
    }
    shown_ = 0;
    const double full = level_full_;
    // heat: v.temp.stress > onset ? (v.temp.v < comfort_low ? sky : cut) : life, fill = stress, v.toFixed(1)+'°'
    Row& heat = heat_;
    heat.shown = body.temp.counts;
    if (heat.shown) {
        heat.tone = body.temp_stress > stress_onset_ ? (body.temp.value < comfort_low_ ? sky_ : cut_) : life_;
        heat.fill = ase::math::clamp(body.temp_stress, 0.0, 1.0);
        if (!set_number(heat, hud_decimals(body.temp.value, static_cast<int32_t>(temp_decimals_)), failed)) {
            return false;
        }
    }
    // hunger, thirst: of level_full, Math.round(v)+' %'
    Row& hunger = hunger_;
    hunger.shown = body.hunger.counts;
    if (hunger.shown) {
        hunger.tone = ground_;
        hunger.fill = ase::math::clamp(body.hunger.value / full, 0.0, 1.0);
        if (!set_number(hunger, hud_percent(body.hunger.value / full), failed)) {
            return false;
        }
    }
    Row& thirst = thirst_;
    thirst.shown = body.thirst.counts;
    if (thirst.shown) {
        thirst.tone = water_;
        thirst.fill = ase::math::clamp(body.thirst.value / full, 0.0, 1.0);
        if (!set_number(thirst, hud_percent(body.thirst.value / full), failed)) {
            return false;
        }
    }
    // energy: of level_full, Math.round(v)+' / 100'
    Row& energy = energy_;
    energy.shown = body.energy.counts;
    if (energy.shown) {
        energy.tone = time_;
        energy.fill = ase::math::clamp(body.energy.value / full, 0.0, 1.0);
        if (!set_number(energy, hud_rounded(body.energy.value, 0), failed)) {
            return false;
        }
    }
    // fatigue: a share, Math.round(f*100)+' %'
    Row& fatigue = fatigue_;
    fatigue.shown = body.fatigue.counts;
    if (fatigue.shown) {
        fatigue.tone = sky_;
        fatigue.fill = ase::math::clamp(body.fatigue.value, 0.0, 1.0);
        if (!set_number(fatigue, hud_percent(body.fatigue.value), failed)) {
            return false;
        }
    }
    // health: a share 0 to 1 by its owner (MTB_HEALTH in hub_metrics.json), Math.round(v)+' %' on the scale of
    // level_full; alert under health_alert of that scale, otherwise life
    Row& health = health_;
    health.shown = body.health.counts;
    if (health.shown) {
        health.tone = body.health.value * full < health_alert_ ? alert_ : life_;
        health.fill = ase::math::clamp(body.health.value, 0.0, 1.0);
        if (!set_number(health, hud_percent(body.health.value), failed)) {
            return false;
        }
    }
    for (const Row* row : {&heat_, &hunger_, &thirst_, &energy_, &fatigue_, &health_}) {
        shown_ += row->shown ? 1 : 0;
    }
    return true;
}

void SurvivalBodyRail::stroke_glyph(UiPen& canvas, const ::godot::Transform2D& frame,
                                    const std::vector<UiSvgOutline>& outlines, const ::godot::Vector2& centre,
                                    double size, const ::godot::Color& color, double width, double joined) const {
    // GL[name](x, y, k, c): the path data is written for k = 1 about the origin; the mockup scales it by k about
    // (x, y) and keeps the stroke width, so the points move and the width stays. Its open strokes are L() lines,
    // stroke-linecap round; GL.health names stroke-linejoin round for its closed path.
    const float scale = static_cast<float>(size);
    for (const UiSvgOutline& outline : outlines) {
        UiSvgOutline placed;
        placed.closed = outline.closed;
        for (int64_t at = 0; at < outline.points.size(); ++at) {
            placed.points.push_back(centre + outline.points[at] * scale);
        }
        if (joined > 0.0) {
            svg_stroke_joined(canvas, frame, placed, color, width);
        } else {
            svg_stroke_round(canvas, frame, placed, color, width);
        }
    }
}

void SurvivalBodyRail::draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still) {
    if (theme_ == nullptr) {
        return;
    }
    canvas.draw_set_transform_matrix(frame);
    // L([[x, top], [x, bottom]], {col: --hud3}): the rail's line, outside the rows' fade.
    ::godot::PackedVector2Array rail;
    rail.push_back(::godot::Vector2(static_cast<float>(x_), static_cast<float>(top_)));
    rail.push_back(::godot::Vector2(static_cast<float>(x_), static_cast<float>(bottom_)));
    svg_polyline_round(canvas, frame, rail, faint_, line_.width());

    // T(x, label_y, 'CORPO', {z, a: start, tr: rotate(turn x label_y), col: --hud2})
    UiTextPlace word;
    word.at = ::godot::Vector2(static_cast<float>(x_), static_cast<float>(label_y_));
    word.size = label_size_;
    word.anchor = GODOT_TEXT_ANCHOR_START;
    word.bare = true;  // s.html T writes class o.c without sh: no outline
    word.color = soft_;
    word.opacity = 1.0;
    word.elapsed = elapsed;
    word.still = still;
    word.rotate_deg = label_turn_;
    word.pivot = word.at;
    label_.draw(canvas, frame, word);

    double place = 0.0;
    for (Row* walked : {&heat_, &hunger_, &thirst_, &energy_, &fatigue_, &health_}) {
        Row& row = *walked;
        if (!row.shown) {
            continue;
        }
        const double y = row_y_ + place * row_step_;
        const double delay = row_delay_ * place;
        const double share = fade_.progress(elapsed, delay, still);
        canvas.draw_set_transform_matrix(frame);
        const ::godot::Color tone = faded(row.tone, share);
        const ::godot::Vector2 centre(static_cast<float>(x_), static_cast<float>(y));
        stroke_glyph(canvas, frame, row.stem, centre, glyph_size_, tone, stem_line_, row.joined);
        stroke_glyph(canvas, frame, row.glyph, centre, glyph_size_, tone, glyph_line_, row.joined);

        // L([[left, y+dy], [right, y+dy]], {col: --hud3}) and L([[right - span*fill, y+dy], [right, y+dy]], {w})
        const float track_y = static_cast<float>(y + track_dy_);
        ::godot::PackedVector2Array track;
        track.push_back(::godot::Vector2(static_cast<float>(track_left_), track_y));
        track.push_back(::godot::Vector2(static_cast<float>(track_right_), track_y));
        svg_polyline_round(canvas, frame, track, faded(faint_, share), line_.width());
        ::godot::PackedVector2Array fill;
        fill.push_back(
            ::godot::Vector2(static_cast<float>(track_right_ - (track_right_ - track_left_) * row.fill), track_y));
        fill.push_back(::godot::Vector2(static_cast<float>(track_right_), track_y));
        svg_polyline_round(canvas, frame, fill, tone, fill_line_);

        // T(name_x, y+dy, word, {z, a: end, col: tone}) and T(number_x, y+dy, value, {c: d, z, col: --hud2})
        UiTextPlace name;
        name.at = ::godot::Vector2(static_cast<float>(name_x_), static_cast<float>(y + text_dy_));
        name.size = name_size_;
        name.anchor = GODOT_TEXT_ANCHOR_END;
        name.bare = true;  // s.html T writes class o.c without sh: no outline
        name.color = row.tone;
        name.opacity = 1.0;
        name.fade = true;
        name.delay = delay;
        name.elapsed = elapsed;
        name.still = still;
        row.name.draw(canvas, frame, name);
        UiTextPlace number;
        number.at = ::godot::Vector2(static_cast<float>(number_x_), static_cast<float>(y + text_dy_));
        number.size = number_size_;
        number.anchor = GODOT_TEXT_ANCHOR_START;
        number.bare = true;  // s.html T writes class o.c without sh: no outline
        number.color = soft_;
        number.opacity = 1.0;
        number.fade = true;
        number.delay = delay;
        number.elapsed = elapsed;
        number.still = still;
        row.number.draw(canvas, frame, number);
        place += 1.0;
    }

    // GL.pulse(x, pulse_y, pulse_size, i ? --hud : --hud2)
    canvas.draw_set_transform_matrix(frame);
    // GL.pulse is one open L() line: round ends and round knees, as every open stroke.
    stroke_glyph(canvas, frame, pulse_, ::godot::Vector2(static_cast<float>(x_), static_cast<float>(pulse_y_)),
                 pulse_size_, shown_ > 0 ? ink_ : soft_, glyph_line_, 0.0);
}

bool SurvivalBodyRail::hit_row(const ::godot::Vector2& point, int32_t& index) const {
    // The one target per shown row is row_place(); a tap is asked on the same rect.
    ::godot::Rect2 target;
    for (size_t place = 0; row_place(place, target); ++place) {
        if (point.x >= target.position.x && point.x <= target.get_end().x && point.y >= target.position.y &&
            point.y <= target.get_end().y) {
            index = static_cast<int32_t>(place);
            return true;
        }
    }
    return false;
}

bool SurvivalBodyRail::row_place(size_t index, ::godot::Rect2& place) const {
    // rect x=track_left y=row-hit_h/2 width=hit_w height=hit_h: centred on the row's y.
    if (index >= shown_) {
        return false;
    }
    const double y = row_y_ + row_step_ * static_cast<double>(index);
    const double half = hit_h_ * GODOT_HALF;
    place = ::godot::Rect2(static_cast<float>(track_left_), static_cast<float>(y - half), static_cast<float>(hit_w_),
                           static_cast<float>(hit_h_));
    return true;
}

}  // namespace ase::adp::godot
