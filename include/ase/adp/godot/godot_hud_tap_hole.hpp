#pragma once

/**
 * ASE GODOT ADAPTER - HUD TAP HOLE
 *
 * @file        godot_hud_tap_hole.hpp
 * @brief       The tap hole in the birch: the elder spout, the bark vessel at the foot and the falling drop
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, element N13 (the mockup's drawWorld, "tap hole in the
 *              birch: elder spout, bark vessel, a drop", m.html 472). It stands in the world group: drawn after the
 *              things on the ground and the tool place, under the brightness WorldDrop gives the world layer.
 *
 *              THE DRAWING. While a tap runs and the birch lives, the spout leaves the trunk at its foot plus
 *              hud_world_tap_spout_dx, at the height the tap hits (hud_world_tap_share of the drawn tree, never
 *              lower than hud_world_tap_min), as a stroke hud_world_tap_spout_line wide in MATERIAL_PITH,
 *              hud_world_tap_spout_len along and hud_world_tap_spout_rise down. The bark vessel hangs at the foot
 *              under the spout's mouth, scaled by hud_world_tap_vessel_k, its foot hud_world_tap_vessel_dy below
 *              the trunk's, its sap body at what it holds (BarkVessel). A drop of SAP_DROP,
 *              hud_world_tap_drop_radius round, falls from the mouth (hud_world_tap_drop_dx, _dy) to
 *              hud_world_tap_drop_gap above the foot, hud_world_tap_drop_rate falls a second.
 *
 *              THE PULSE AT THE HOLE. In the modes sap and plug the mockup's render lays a pulse where the tap
 *              hits (m.html 909): a ring hud_world_tap_pulse_radius round in the tone flow, hud_line wide, at the
 *              foot plus hud_world_tap_dx and the height of the spout. It is a layer of the HUD, not of the world
 *              group: hud.modes names it as tap_pulse in the layers of sap and plug, and VivariumHud draws it there
 *              with draw_pulse after the world's brightness has ended, pulsing as .pulse from the mode's opening.
 *              It stands whether a tap runs or not, as the mockup draws it, and only while the birch is placed.
 *
 *              FULL FROM THE FRAME. Whether a tap runs, what the vessel holds and what it holds at most are the
 *              tap's state on flora.sap.v1, and whether the vessel hangs at the trunk is ase-inventory's on
 *              flora.carry.v1. TapHole draws whatever the frame carries and never asks whether a port delivered:
 *              until they do, the bridge writes the mockup's tap from hud_bridge_patch_sap_* into the frame
 *              (VivariumView::fill_hud_world) and VivariumView::open_gap marks N0457 in its own source.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:101609 - tap birch sap
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0457 (flora.sap.v1), N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_bark_vessel.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>

namespace ase::adp::godot {

/** What the tap hole shows this frame. */
struct TapHoleFrame {
    bool running = false;        // S.tap && !S.dead: a tap runs in a living birch
    bool placed = false;         // the birch stands on the screen
    ::godot::Vector2 trunk;      // the foot of the trunk in mockup units (TX, TY())
    double tree_height = 0.0;    // treeH() in mockup units
    bool vessel = false;         // the bark vessel hangs at the trunk (o.c === 'trunk')
    ItemWidgetCarry carry;       // the vessel's held_l and capacity_l (S.tap.l, its capacity)
    double clock = 0.0;          // the seconds of the view's clock, the phase of the drop
};

/** The tap hole in the birch. */
class TapHole {
public:
    /** Binds the measures, the colours and the vessel; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the tap of `now` under `frame`. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const TapHoleFrame& now) const;

    /** Draws the pulse at the hole of `now` under `frame`, `elapsed` seconds after the mode opened. */
    void draw_pulse(UiPen& canvas, const ::godot::Transform2D& frame, const TapHoleFrame& now, double elapsed,
                    bool still) const;

private:
    BarkVessel vessel_;
    UiRing ring_;                // the pulse of the hole
    double pulse_dx_ = 0.0;      // hud_world_tap_dx
    double pulse_radius_ = 0.0;  // hud_world_tap_pulse_radius
    double pulse_line_ = 0.0;    // hud_line
    ::godot::Color flow_;        // the tone flow
    double share_ = 0.0;         // hud_world_tap_share
    double lowest_ = 0.0;        // hud_world_tap_min
    double spout_dx_ = 0.0;      // hud_world_tap_spout_dx
    double spout_len_ = 0.0;     // hud_world_tap_spout_len
    double spout_rise_ = 0.0;    // hud_world_tap_spout_rise
    double spout_line_ = 0.0;    // hud_world_tap_spout_line
    double vessel_k_ = 0.0;      // hud_world_tap_vessel_k
    double vessel_dy_ = 0.0;     // hud_world_tap_vessel_dy
    double drop_dx_ = 0.0;       // hud_world_tap_drop_dx
    double drop_dy_ = 0.0;       // hud_world_tap_drop_dy
    double drop_radius_ = 0.0;   // hud_world_tap_drop_radius
    double drop_rate_ = 0.0;     // hud_world_tap_drop_rate
    double drop_gap_ = 0.0;      // hud_world_tap_drop_gap
    ::godot::Color pith_;        // MATERIAL_PITH
    ::godot::Color drop_;        // SAP_DROP
};

}  // namespace ase::adp::godot
