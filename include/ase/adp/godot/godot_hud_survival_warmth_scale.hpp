#pragma once

/**
 * ASE GODOT ADAPTER - HUD SURVIVAL WARMTH SCALE
 *
 * @file        godot_hud_survival_warmth_scale.hpp
 * @brief       The warmth scale CONFORTO over the heat row of the body rail, drawn as the survival mockup's corpo(st)
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U03 (mockup VIVARIUM_PHASE_08_SURVIVAL_HUD_V1,
 *              s.html 205 to 209, element S3, the body card on a leader line from the rail). While the felt
 *              temperature counts for the figure the mockup draws, about the heat row's y
 *              (hud_survival_rail_row_y), in one group fading in with the view (class fade):
 *
 *                leader   a polyline from (hud_survival_comfort_leader_x, y) over
 *                         (hud_survival_comfort_leader_bend, y - rise) to (hud_survival_comfort_right, y - rise),
 *                         rise = hud_survival_comfort_rise, in DOMAIN_CUT at hud_survival_comfort_leader_opacity
 *                         and the line width hud_line
 *                scale    a line from hud_survival_comfort_left to hud_survival_comfort_right at y - rise in
 *                         OVERLAY_INK_FAINT (--hud3); a temperature t stands at
 *                         left + t / hud_survival_comfort_span * (right - left)
 *                band     the comfort band from hud_survival_rail_comfort_low to hud_survival_rail_comfort_high
 *                         degrees on the scale, in DOMAIN_LIFE at hud_survival_comfort_band_line
 *                marker   a triangle filled in OVERLAY_INK (--hud), its tip hud_survival_comfort_marker_gap over the
 *                         scale at the felt temperature, hud_survival_comfort_marker_h high and
 *                         2 * hud_survival_comfort_marker_half wide at its base
 *                bounds   the band's ends as numbers (survival.body.bound) in the data font, centred under them at
 *                         y - rise + hud_survival_comfort_tick_dy, size hud_survival_comfort_tick_size, in
 *                         OVERLAY_INK_FAINT
 *                label    the word survival.body.comfort (CONFORTO) start-anchored at (left,
 *                         y - rise - hud_survival_comfort_label_dy), size hud_survival_comfort_label_size, in
 *                         OVERLAY_INK_SOFT (--hud2)
 *
 *              The felt temperature is MTB_EFFECTIVE_TEMP of ase-metabolism, which its owner takes from the local
 *              temperature of ase-gis (GIS_LOCAL_TEMP). The scale draws what the frame says; it never asks a port
 *              (the bridge writes the frame, gap N0530 under the carrier N0448).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0528 (survival rows of the inventory), N0530 (facets without a hub value), carrier N0448
 * @created     2026-11-20
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_survival_body_rail.hpp>
#include <ase/adp/godot/godot_hud_ui_leader_line.hpp>
#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The warmth scale CONFORTO over the heat row of the body rail. */
class SurvivalWarmthScale {
public:
    /** Binds every hud_survival_comfort_* token, the rail's row and band tokens, the inks and the words. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The scale of `body` for this frame: shown while the felt temperature counts; false with the reason. */
    bool set(const SurvivalBodyFrame& body, std::string& failed);

    /** Draws the scale under `frame`, `elapsed` seconds after the view appeared; `still` holds every motion. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still);

private:
    /** The x of `degrees` on the scale: left + degrees / span * (right - left), the mockup's t2x. */
    double place_of(double degrees) const;

    /** Writes `degrees` as a bound number (survival.body.bound) into `block`; false with the reason. */
    bool set_bound(UiTextBlock& block, double degrees, std::string& failed);

    const UiTheme* theme_ = nullptr;
    UiLeaderLine line_;              // the leader, the scale and the band; the line width hud_line
    UiMotion fade_;                  // hud_anim_fade: the group's fade-in
    UiTextBlock label_;              // survival.body.comfort, class t
    UiTextBlock low_;                // the band's low end in the data font
    UiTextBlock high_;               // the band's high end in the data font
    std::vector<HudRun> bound_;      // survival.body.bound
    ::godot::Color ink_;             // OVERLAY_INK: --hud
    ::godot::Color faint_;           // OVERLAY_INK_FAINT: --hud3
    ::godot::Color soft_;            // OVERLAY_INK_SOFT: --hud2
    ::godot::Color cut_;             // DOMAIN_CUT: the leader
    ::godot::Color life_;            // DOMAIN_LIFE: the comfort band
    double row_y_ = 0.0;             // hud_survival_rail_row_y: the heat row the scale hangs over
    double comfort_low_ = 0.0;       // hud_survival_rail_comfort_low
    double comfort_high_ = 0.0;      // hud_survival_rail_comfort_high
    double left_ = 0.0;              // hud_survival_comfort_left
    double right_ = 0.0;             // hud_survival_comfort_right
    double span_ = 0.0;              // hud_survival_comfort_span
    double rise_ = 0.0;              // hud_survival_comfort_rise
    double leader_x_ = 0.0;          // hud_survival_comfort_leader_x
    double leader_bend_ = 0.0;       // hud_survival_comfort_leader_bend
    double leader_opacity_ = 0.0;    // hud_survival_comfort_leader_opacity
    double band_line_ = 0.0;         // hud_survival_comfort_band_line
    double marker_gap_ = 0.0;        // hud_survival_comfort_marker_gap
    double marker_h_ = 0.0;          // hud_survival_comfort_marker_h
    double marker_half_ = 0.0;       // hud_survival_comfort_marker_half
    double tick_dy_ = 0.0;           // hud_survival_comfort_tick_dy
    double tick_size_ = 0.0;         // hud_survival_comfort_tick_size
    double label_dy_ = 0.0;          // hud_survival_comfort_label_dy
    double label_size_ = 0.0;        // hud_survival_comfort_label_size
    double felt_ = 0.0;              // MTB_EFFECTIVE_TEMP of this frame
    bool shown_ = false;             // the felt temperature counts this frame
};

}  // namespace ase::adp::godot
