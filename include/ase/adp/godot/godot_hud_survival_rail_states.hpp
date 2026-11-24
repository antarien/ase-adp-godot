#pragma once

/**
 * ASE GODOT ADAPTER - HUD SURVIVAL RAIL STATES
 *
 * @file        godot_hud_survival_rail_states.hpp
 * @brief       The states of the body on the rail of signs SINAIS, drawn as the survival mockup's states(st) does
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, row U02 (mockup VIVARIUM_PHASE_08_SURVIVAL_HUD_V1,
 *              s.html 187 to 191, element S2). The states stand on the same rail as the signs of Phase 06
 *              (UiEdgeRail), under them, in the order poisoned, bleeding, emaciated, well rested. The mockup draws:
 *
 *                alert    while at least one shown state is alerting: a bar at (hud_rail_alert_x,
 *                         hud_rail_state_alert_y), hud_rail_alert_w wide and hud_rail_alert_base + alerts *
 *                         hud_rail_alert_step high, in DOMAIN_ALERT (--alert), outside the fade
 *                states   shown state i at y = hud_rail_state_y + i * hud_rail_state_step, fading in (class fade,
 *                         hud_anim_fade): a dot at hud_rail_x of radius hud_rail_state_radius filled and stroked
 *                         (hud_rail_state_line) in the colour its domain names (hud_rail_state_<name>_domain);
 *                         an alerting state (hud_rail_state_<name>_alert 1) carries a pulse ring of
 *                         hud_rail_state_pulse_radius at hud_rail_state_pulse_line; its glyph at
 *                         (hud_rail_state_glyph_x, y), size hud_rail_state_glyph_size, in three parts of one unit:
 *                         line stroked at hud_survival_glyph_line, mark stroked at hud_rail_state_<name>_mark_line,
 *                         fill filled at hud_rail_state_<name>_fill_opacity and stroked at
 *                         hud_rail_state_<name>_fill_line where that is above 0
 *
 *              The states carry no tap: s.html 242 takes clicks only on the buttons of the bench. Which state shows
 *              is the record's: SurvivalBodyFrame carries the four values TAG_MTB_POISONED,
 *              TAG_MTB_BLEEDING, TAG_MTB_EMACIATED and TAG_MTB_WELL_RESTED, which HubThrsValSystem writes per figure
 *              as 1 or 0 by the rules of hub_thresholds.json and ase-pl-survival exports on survival.body.v1
 *              (SurvivalHostBodyExptSystem); the class computes none.
 *
 *              TWO STATES HAVE NO CASE IN THE WORLD YET (carrier N0448): well rested is 0 in every run, because the
 *              figure's plan set in ase-bdi answers its rest goal with a wait and never with a sleep, so
 *              MetabolismBmrSystem never reaches MIN_SLEEP_HOURS_FOR_BMR (D2601, ase-char); poisoned waits on the
 *              gustatory signature of the toadstool (N0592, ase-survival). The class draws both from the record
 *              as for every state and shows 0 as 0; the gap stands here and in the register, never in the picture.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0528 (survival rows of the inventory), N0530 (facets without a hub value), D2601 (no sleep plan of
 *              the figure), N0592 (toadstool taste), carrier N0448
 * @created     2026-11-20
 * @modified    2026-11-23
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_survival_body_rail.hpp>
#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The states of the body under the signs on the rail SINAIS. */
class SurvivalRailStates {
public:
    /** Binds every hud_rail_state_* token, the rail's column and bar, the domain colours and the primitives. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The states `body` shows for this frame; false with the reason in `failed`. */
    bool set(const SurvivalBodyFrame& body, std::string& failed);

    /** Draws the shown states under `frame`, `elapsed` seconds after the view appeared; `still` holds every motion. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, double elapsed, bool still) const;

private:
    /** One state's look, bound from hud_rail_state_<name>_*, and whether this frame shows it. */
    struct State {
        ::godot::Color tone;                 // the colour hud_rail_state_<name>_domain names: DOM[g.dom]
        bool alert = false;                  // hud_rail_state_<name>_alert: g.alert, it pulses and counts into the bar
        std::vector<UiSvgOutline> line;      // hud_rail_state_<name>_line: stroked at hud_survival_glyph_line
        std::vector<UiSvgOutline> mark;      // hud_rail_state_<name>_mark: stroked at mark_line
        double mark_line = 0.0;              // hud_rail_state_<name>_mark_line
        std::vector<UiSvgOutline> fill;      // hud_rail_state_<name>_fill: filled at fill_opacity
        double fill_opacity = 0.0;           // hud_rail_state_<name>_fill_opacity
        double fill_line = 0.0;              // hud_rail_state_<name>_fill_line: 0 draws no stroke
        bool shown = false;                  // the record carries the mark on the figure this frame
    };

    /** Binds `state` to the tokens hud_rail_state_<name>_* of `theme`. */
    static bool bind_state(const UiTheme& theme, State& state, const std::string& name, std::string& failed);

    /**
     * Strokes `outlines` scaled by `size` about `centre` in `color` at `width`, laid in the units of `frame`:
     * GL[name](x, y, k, c).
     */
    void stroke(UiPen& canvas, const ::godot::Transform2D& frame, const std::vector<UiSvgOutline>& outlines,
                const ::godot::Vector2& centre, double size, const ::godot::Color& color, double width) const;

    /** Fills `outlines` scaled by `size` about `centre` in `color`. */
    void fill(UiPen& canvas, const std::vector<UiSvgOutline>& outlines, const ::godot::Vector2& centre, double size,
              const ::godot::Color& color) const;

    const UiTheme* theme_ = nullptr;
    UiRing ring_;                    // the dots and the pulse
    UiMotion fade_;                  // hud_anim_fade: class fade of each state's group
    ::godot::Color alert_;           // DOMAIN_ALERT: --alert, the bar of the states
    double x_ = 0.0;                 // hud_rail_x: the rail's column, the dots' x
    double alert_x_ = 0.0;           // hud_rail_alert_x
    double alert_w_ = 0.0;           // hud_rail_alert_w
    double alert_base_ = 0.0;        // hud_rail_alert_base
    double alert_step_ = 0.0;        // hud_rail_alert_step
    double alert_y_ = 0.0;           // hud_rail_state_alert_y: the bar of the states, apart from the signs' bar
    double y_ = 0.0;                 // hud_rail_state_y: the first state
    double step_ = 0.0;              // hud_rail_state_step
    double radius_ = 0.0;            // hud_rail_state_radius
    double dot_line_ = 0.0;          // hud_rail_state_line: the dot's stroke
    double pulse_radius_ = 0.0;      // hud_rail_state_pulse_radius
    double pulse_line_ = 0.0;        // hud_rail_state_pulse_line
    double glyph_x_ = 0.0;           // hud_rail_state_glyph_x
    double glyph_size_ = 0.0;        // hud_rail_state_glyph_size
    double glyph_line_ = 0.0;        // hud_survival_glyph_line: the stroke of a glyph's line part
    State poison_;                   // TAG_MTB_POISONED
    State bleed_;                    // TAG_MTB_BLEEDING
    State emaciated_;                // TAG_MTB_EMACIATED
    State rested_;                   // TAG_MTB_WELL_RESTED
};

}  // namespace ase::adp::godot
