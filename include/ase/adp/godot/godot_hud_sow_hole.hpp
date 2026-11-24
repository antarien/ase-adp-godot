#pragma once

/**
 * ASE GODOT ADAPTER - HUD SOW HOLE
 *
 * @file        godot_hud_sow_hole.hpp
 * @brief       The hole the dibber made: a dark ellipse at the stem place until the seed closes it
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, element N14 (the mockup's drawWorld, "the hole the
 *              dibber made, before the seed closes it", m.html 476). It stands in the world group: drawn after the
 *              tap hole, under the brightness WorldDrop gives the world layer.
 *
 *              THE DRAWING. For hud_world_sow_hole_ms after a seed went in, an ellipse hud_world_sow_hole_rx wide
 *              and hud_world_sow_hole_ry high in WORLD_DIBBER_HOLE stands where the place of the sowing meets the
 *              screen (UiAnchor::place); a place behind the camera draws nothing.
 *
 *              FULL FROM THE FRAME. Where and when a seed went in is ase-flora's answer to the sowing on
 *              flora.seed.v1. SowHole draws whatever the frame carries and never asks whether the port delivered:
 *              until it does, the view writes the stem place and the clock of a sowing the bridge accepted
 *              (VivariumCarryBridge::apply, ASK_SOW) into the frame and VivariumView::open_gap marks the gap.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100388 - sow a new seed
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0451 (flora.carry.v1 seed taken), carrier N0448
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_anchor.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <string>

namespace ase::adp::godot {

/** What the sow hole shows this frame. */
struct SowHoleFrame {
    const UiAnchor* anchor = nullptr;  // the projection of this frame
    bool sown = false;                 // a seed went in (S.hole was set)
    ::godot::Vector3 place;            // where it went in, world units (U0, Z0 on the ground)
    double age = 0.0;                  // seconds since it went in
    bool open = false;                 // the spade dug the planting hole and nothing closed it yet (Q02), drawn while open
    bool seeded = false;             // a seed lies in the open planting hole (Q03)
    bool covered = false;              // the planting hole is closed over its seed (Q04); the dibber's sowing closes it
};

/** The hole the dibber made. */
class SowHole {
public:
    /** Binds the measures and the colour; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the hole of `now` under `frame`. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const SowHoleFrame& now) const;

private:
    double open_ms_ = 0.0;   // hud_world_sow_hole_ms
    double rx_ = 0.0;        // hud_world_sow_hole_rx
    double ry_ = 0.0;        // hud_world_sow_hole_ry
    ::godot::Color hole_;    // WORLD_DIBBER_HOLE
};

}  // namespace ase::adp::godot
