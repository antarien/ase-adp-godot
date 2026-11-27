/**
 * ASE GODOT ADAPTER - HUD ARC RING
 *
 * @file        godot_hud_arc_ring.cpp
 * @brief       ArcRing: one rigid disc of the sky arc, turning under the needle, see godot_hud_arc_ring.hpp
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, mockup ringLay and arcFrame.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_arc_ring.hpp>
#include <ase/adp/godot/godot_hud_arc_pure.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/hud/style/types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

namespace {

/** The prefix of a ring's tokens. */
const char* ring_prefix(uint32_t ring) {
    const char* const prefixes[GODOT_ARC_RINGS] = {GODOT_ARC_RING_WORD_DAY, GODOT_ARC_RING_WORD_LUN,
                                                   GODOT_ARC_RING_WORD_YEAR};
    return prefixes[ring];
}

}  // namespace

bool ArcRing::bind(const UiTheme& theme, uint32_t ring, const UiArcFade& fade, const ArcSign& sign,
                   std::string& failed) {
    ring_ = ring;
    fade_ = &fade;
    sign_ = &sign;
    laid_ = false;
    const std::string prefix = ring_prefix(ring);
    double cx = 0.0;
    double cy = 0.0;
    for (const std::pair<std::string, double*>& slot : std::initializer_list<std::pair<std::string, double*>>{
             {"hud_arc_cx", &cx},
             {"hud_arc_cy", &cy},
             {prefix + "r", &radius_},
             {prefix + "half_deg", &half_deg_},
             {prefix + "step_share", &step_share_},
             {prefix + "tick_half", &tick_half_},
             {prefix + "tick_line", &tick_line_},
             {"hud_arc_track_line", &track_line_},
             {"hud_arc_lay_reach", &reach_},
             {"hud_arc_segments_per_unit", &per_unit_},
             {"hud_arc_day_light_line", &light_line_},
             {"hud_arc_day_light_alpha", &light_alpha_},
             {"hud_arc_year_season_line", &season_line_},
             {"hud_arc_year_season_alpha", &season_alpha_},
             {"hud_arc_year_season_gap_deg", &season_gap_deg_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    centre_ = ::godot::Vector2(static_cast<float>(cx), static_cast<float>(cy));
    ::godot::String tone;
    if (!theme.token_text("hud_arc_day_light_tone", tone) || !theme.tone(utf8_of(tone), light_)) {
        failed = "hud_arc_day_light_tone";
        return false;
    }
    if (!theme.color("OVERLAY_INK_FAINT", faint_) || !theme.color("OVERLAY_INK_SOFT", soft_)) {
        failed = "OVERLAY_INK_FAINT, OVERLAY_INK_SOFT";
        return false;
    }
    return !(radius_ <= 0.0) && !(half_deg_ <= 0.0) && !(step_share_ <= 0.0);
}

::godot::Vector2 ArcRing::low_point() const {
    return UiRing::point(centre_, radius_, arc_ring_angle(0.0));
}

::godot::PackedVector2Array ArcRing::arc_points(double from_deg, double to_deg, double scale) const {
    const double length = radius_ * (to_deg - from_deg) * static_cast<double>(ase::math::DEG_TO_RAD);
    int64_t count = static_cast<int64_t>(ase::math::ceil(static_cast<float>(length * per_unit_ * scale)));
    count = count < GODOT_ARC_POINTS_MIN ? GODOT_ARC_POINTS_MIN : count;
    ::godot::PackedVector2Array points;
    for (int64_t index = 0; index <= count; ++index) {
        const double degrees = from_deg + (to_deg - from_deg) * static_cast<double>(index) / static_cast<double>(count);
        points.push_back(UiRing::point(centre_, radius_, arc_ring_angle(degrees)));
    }
    return points;
}

void ArcRing::lay_grid(double scale) {
    const double reach_deg = reach_ * half_deg_;
    strokes_.push_back(Stroke{arc_points(-reach_deg, reach_deg, scale), faint_, track_line_});
    const int64_t last = static_cast<int64_t>(ase::math::floor(static_cast<float>(reach_ / step_share_)));
    for (int64_t step = -last; step <= last; ++step) {
        const double degrees = static_cast<double>(step) * step_share_ * half_deg_;
        ::godot::PackedVector2Array tick;
        tick.push_back(UiRing::point(centre_, radius_ - tick_half_, arc_ring_angle(degrees)));
        tick.push_back(UiRing::point(centre_, radius_ + tick_half_, arc_ring_angle(degrees)));
        strokes_.push_back(Stroke{tick, soft_, tick_line_});
    }
}

void ArcRing::lay_span(const ArcLaidSign& sign, double scale) {
    if (!(sign.span > 0.0)) {
        return;
    }
    const bool light = sign.kind == GODOT_ARC_KIND_DAWN;
    const bool season = sign.kind == GODOT_ARC_KIND_SEASON;
    if (!light && !season) {
        return;
    }
    const double gap = season ? season_gap_deg_ : 0.0;
    const double from = sign.share * half_deg_ + gap;
    const double to = (sign.share + sign.span) * half_deg_ - gap;
    if (!(to > from)) {
        return;
    }
    ::godot::Color color = light ? light_ : sign.tone;
    color.a = static_cast<float>(light ? light_alpha_ : season_alpha_);
    strokes_.push_back(Stroke{arc_points(from, to, scale), color, light ? light_line_ : season_line_});
}

bool ArcRing::lay(const ::godot::Transform2D& device, const ArcRingFrame& now, uint64_t seq, std::string& failed) {
    const double scale = static_cast<double>(ase::math::abs(device.get_scale().x));
    strokes_.clear();
    signs_.clear();
    lay_grid(scale);
    signs_.reserve(now.signs.size());
    for (const ArcSignFrame& frame : now.signs) {
        ArcLaidSign laid;
        if (!sign_->lay(frame, ring_, laid, failed)) {
            return false;
        }
        lay_span(laid, scale);
        signs_.push_back(laid);
    }
    laid_seq_ = seq;
    laid_scale_ = scale;
    laid_ = true;
    return true;
}

void ArcRing::draw_stroke(UiPen& canvas, const Stroke& stroke, const ::godot::Transform2D& turn) const {
    const int64_t count = stroke.points.size();
    if (count == 2) {
        const ::godot::Vector2 middle = turn.xform((stroke.points[0] + stroke.points[1]) * GODOT_HALF);
        const double opacity = fade_->opacity(static_cast<double>(middle.x));
        if (opacity > 0.0) {
            canvas.draw_line(stroke.points[0], stroke.points[1], UiArcFade::faded(stroke.color, opacity),
                             static_cast<float>(stroke.width), true);
        }
        return;
    }
    ::godot::PackedColorArray colors;
    colors.resize(count);
    for (int64_t index = 0; index < count; ++index) {
        colors.set(index, UiArcFade::faded(stroke.color, fade_->opacity(static_cast<double>(
                                                              turn.xform(stroke.points[index]).x))));
    }
    canvas.draw_polyline_colors(stroke.points, colors, static_cast<float>(stroke.width));
}

bool ArcRing::draw(UiPen& canvas, const ::godot::Transform2D& frame, const ArcRingFrame& now, uint64_t seq,
                   double elapsed, bool still, std::vector<std::string>& flank, std::string& failed) {
    const double scale = static_cast<double>(ase::math::abs((canvas.screen() * frame).get_scale().x));
    if (!laid_ || laid_seq_ != seq || laid_scale_ != scale) {
        if (!lay(canvas.screen() * frame, now, seq, failed)) {
            return false;
        }
    }
    // THE WHOLE DISC TURNS: clockwise about the common centre by the share of now times the half span, so a point
    // laid at share s stands (s - now) times that span from the needle.
    const float turn_rad = static_cast<float>(now.now * half_deg_) * ase::math::DEG_TO_RAD;
    const ::godot::Transform2D turn = ::godot::Transform2D(0.0f, centre_) *
                                      ::godot::Transform2D(turn_rad, ::godot::Vector2()) *
                                      ::godot::Transform2D(0.0f, -centre_);
    canvas.draw_set_transform_matrix(frame * turn);
    for (const Stroke& stroke : strokes_) {
        draw_stroke(canvas, stroke, turn);
    }
    canvas.draw_set_transform_matrix(frame);
    ArcSignPlace place;
    place.centre = centre_;
    place.radius = radius_;
    place.half_deg = half_deg_;
    place.now = now.now;
    place.ring = ring_;
    place.fade = fade_;
    place.elapsed = elapsed;
    place.still = still;
    for (ArcLaidSign& sign : signs_) {
        sign_->draw(canvas, frame, place, sign, flank);
    }
    return true;
}

}  // namespace ase::adp::godot
