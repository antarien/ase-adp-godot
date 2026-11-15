#pragma once

/**
 * ASE GODOT ADAPTER - HUD ROOT CUT
 *
 * @file        godot_hud_root_cut.hpp
 * @brief       The ground opened under the trunk in the water mode: soil, held water, the level after a pour, roots
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block RootCut (the mockup's drawRootCut, drawn in the
 *              water mode after the ground and before the tree).
 *
 *              THE CUT. A half ellipse hud_root_cut_rx by hud_root_cut_ry whose centre lies hud_root_cut_dy below
 *              the trunk (UiAnchor::trunk), drawn as hud_root_cut_arc_points points along its rim. The soil runs
 *              from ROOT_CUT_SOIL_TOP at the trunk to ROOT_CUT_SOIL_DEEP one hud_root_cut_ry further down; the
 *              floor of the cut lies within hud_root_cut_floor_pad below that. The rim is DOMAIN_WATER_RIM at
 *              hud_root_cut_rim_line.
 *
 *              THE WATER. The soil water over field capacity of the patch record (flora.patch.v1, soil_rel) fills
 *              the cut from its level, measured down from the top, to the floor: DOMAIN_WATER_HELD at the level
 *              to DOMAIN_WATER_DEEP at the floor. The level a pour would reach is a dashed line in DOMAIN_WATER
 *              (hud_root_cut_pour_dash, hud_root_cut_pour_line) across the cut.
 *
 *              THE ROOTS. The segments the root module grows for the plant, each with both ends in the world and
 *              its branch order, placed over UiAnchor::place and cut to the half ellipse; a segment of order n is
 *              hud_root_cut_root_width minus n times hud_root_cut_root_width_step wide, never thinner than
 *              hud_root_cut_root_width_min, in ROOT_CUT_ROOT. The unit grows no root of its own.
 *
 *              THE STATE COMES FROM THE FRAME. The segments come from ase-radix over flora.root.v1 (N0463), the
 *              level after a pour from the water budget into the patch record (N0454). The cut draws what the
 *              frame carries and never asks whether a port delivered: while one is silent the bridge fills the
 *              mockup's values (its root tree and S.pour) and VivariumView::open_gap logs the gap on its edge.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0463 (flora.root.v1), N0454 (flora.patch.v1 level after a pour), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_anchor.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One segment of a root as the root module grows it. */
struct RootSegment {
    ::godot::Vector3 from;   // the end towards the stem base, world units
    ::godot::Vector3 to;     // the end towards the tip, world units
    uint32_t order = 0;      // the branch order, 0 at the tap root
};

/** What the root cut shows this frame. */
struct RootCutFrame {
    const UiAnchor* anchor = nullptr;               // the projection of this frame
    PlantOrigin origin;                             // the stem base of the plant in the cut
    double soil_rel = 0.0;                          // soil water over field capacity, 0..1
    double pour_rel = 0.0;                          // the share a pour adds to soil_rel, 0 while none is planned
    const std::vector<RootSegment>* roots = nullptr;// the segments, nullptr while the plant has none
};

/** The ground opened under the trunk. */
class RootCut {
public:
    /** Binds measures and colours; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the cut of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const RootCutFrame& now,
              std::string& failed);

private:
    /** The rim point at `share` of the half turn from the right end, around `centre`. */
    ::godot::Vector2 rim(const ::godot::Vector2& centre, double share) const;

    /** The colour of the band from `top` to `top + span` at height `y`, from `upper` to `lower`. */
    static ::godot::Color ramp(const ::godot::Color& upper, const ::godot::Color& lower, double top, double span,
                               double y);

    /** The water from `level` to the floor. */
    void draw_water(UiPen& canvas, const ::godot::Vector2& centre, double trunk_y, double level) const;

    /** `a` to `b` cut to the half ellipse; false when nothing of it lies inside. */
    bool cut(const ::godot::Vector2& centre, ::godot::Vector2& a, ::godot::Vector2& b) const;

    double rx_ = 0.0;                 // hud_root_cut_rx
    double ry_ = 0.0;                 // hud_root_cut_ry
    double dy_ = 0.0;                 // hud_root_cut_dy
    double floor_pad_ = 0.0;          // hud_root_cut_floor_pad
    double arc_points_ = 0.0;         // hud_root_cut_arc_points
    double pour_dash_ = 0.0;          // hud_root_cut_pour_dash
    double pour_line_ = 0.0;          // hud_root_cut_pour_line
    double rim_line_ = 0.0;           // hud_root_cut_rim_line
    double root_width_ = 0.0;         // hud_root_cut_root_width
    double root_width_step_ = 0.0;    // hud_root_cut_root_width_step
    double root_width_min_ = 0.0;     // hud_root_cut_root_width_min
    ::godot::Color soil_top_, soil_deep_, held_, deep_, water_, rim_color_, root_;
};

}  // namespace ase::adp::godot
