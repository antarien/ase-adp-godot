#pragma once

/**
 * ASE GODOT ADAPTER - HUD ARC RING
 *
 * @file        godot_hud_arc_ring.hpp
 * @brief       ArcRing: one rigid disc of the sky arc, its track, ticks, spans and signs, turning under the needle
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, elements A02 to A04, A08 and A11 of the mockup's
 *              ringLay. Three rings share one centre high above the frame (hud_arc_cx, hud_arc_cy), the smallest
 *              (year, hud_arc_year_r) the highest. A ring is RIGID: its content is laid once for a set (seq) and a
 *              device scale and kept as strokes in ring coordinates; each frame only turns it as a whole by the share
 *              of now times the ring's half span (hud_arc_<ring>_half_deg) about the centre, and draws. So a sign at
 *              share s stands (s - now) times that span from the needle, which is arc_disc_angle and nothing else:
 *              the adapter turns what the owner placed, it computes no hour, no day and no clock.
 *
 *              THE TRACK, hud_arc_track_line in OVERLAY_INK_FAINT, runs hud_arc_lay_reach half windows each side of
 *              the anchor. THE TICKS stand every hud_arc_<ring>_step_share of the window from the anchor, hud_arc_<ring>
 *              _tick_half inside and outside the ring at hud_arc_<ring>_tick_line in OVERLAY_INK_SOFT: the owner
 *              anchors a ring on a whole unit of it (HORAX, TUMBRAE), so the ticks of the grid fall on the units.
 *              THE LIGHT of the day ring is the span of the dawn occurrences (hud_arc_day_light_tone,
 *              hud_arc_day_light_line, hud_arc_day_light_alpha); THE SEASONS of the year ring are the spans of the
 *              season occurrences in the colour of their season (hud_arc_year_season_line, _alpha), each stopping
 *              short of its edge by hud_arc_year_season_gap_deg. The arcs get their points from their length and the
 *              device scale (hud_arc_segments_per_unit): a vector at every resolution.
 *
 *              THE ENDS FADE OUT. Godot knows no mask on the canvas: every stroke takes its opacity from UiArcFade at
 *              its place, a polyline by its own points, a tick by its middle, and a sign by its disc.
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

#include <ase/adp/godot/godot_hud_arc_frame.hpp>
#include <ase/adp/godot/godot_hud_arc_sign.hpp>
#include <ase/adp/godot/godot_hud_ui_arc_fade.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One ring. */
class ArcRing {
public:
    /** Binds the ring `ring` (GODOT_ARC_SLOT_*); `fade` and `sign` stay owned by the caller and outlive the ring. */
    bool bind(const UiTheme& theme, uint32_t ring, const UiArcFade& fade, const ArcSign& sign, std::string& failed);

    /** Draws the ring of `now` under `frame`; false with the reason in `failed`. Flank lines go to `flank`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const ArcRingFrame& now, uint64_t seq, double elapsed,
              bool still, std::vector<std::string>& flank, std::string& failed);

    /** The low point of the ring below the common centre, where the needle crosses it. */
    ::godot::Vector2 low_point() const;

private:
    /** One stroke in ring coordinates: two points are a line, more are a polyline. */
    struct Stroke {
        ::godot::PackedVector2Array points;  // the points laid in ring coordinates
        ::godot::Color color;                // its colour before the fade
        double width = 0.0;                  // its line width
    };

    bool lay(const ::godot::Transform2D& device, const ArcRingFrame& now, uint64_t seq, std::string& failed);
    void lay_grid(double scale);
    void lay_span(const ArcLaidSign& sign, double scale);
    ::godot::PackedVector2Array arc_points(double from_deg, double to_deg, double scale) const;
    void draw_stroke(UiPen& canvas, const Stroke& stroke, const ::godot::Transform2D& turn) const;

    ::godot::Vector2 centre_;        // hud_arc_cx, hud_arc_cy
    double radius_ = 0.0;            // hud_arc_<ring>_r
    double half_deg_ = 0.0;          // hud_arc_<ring>_half_deg
    double step_share_ = 0.0;        // hud_arc_<ring>_step_share
    double tick_half_ = 0.0;         // hud_arc_<ring>_tick_half
    double tick_line_ = 0.0;         // hud_arc_<ring>_tick_line
    double track_line_ = 0.0;        // hud_arc_track_line
    double reach_ = 0.0;             // hud_arc_lay_reach
    double per_unit_ = 0.0;          // hud_arc_segments_per_unit
    double light_line_ = 0.0;        // hud_arc_day_light_line
    double light_alpha_ = 0.0;       // hud_arc_day_light_alpha
    double season_line_ = 0.0;       // hud_arc_year_season_line
    double season_alpha_ = 0.0;      // hud_arc_year_season_alpha
    double season_gap_deg_ = 0.0;    // hud_arc_year_season_gap_deg
    ::godot::Color faint_, soft_, light_;  // OVERLAY_INK_FAINT, OVERLAY_INK_SOFT, the tone of hud_arc_day_light_tone
    uint32_t ring_ = 0;              // GODOT_ARC_SLOT_*
    const UiArcFade* fade_ = nullptr;
    const ArcSign* sign_ = nullptr;

    std::vector<Stroke> strokes_;       // the laid track, ticks and spans
    std::vector<ArcLaidSign> signs_;    // the laid signs
    uint64_t laid_seq_ = 0;             // the set the strokes and signs stand for
    double laid_scale_ = 0.0;           // the device scale their points were counted at
    bool laid_ = false;                 // the ring was laid once
};

}  // namespace ase::adp::godot
