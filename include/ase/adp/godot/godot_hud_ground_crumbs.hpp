#pragma once

/**
 * ASE GODOT ADAPTER - HUD GROUND CRUMBS
 *
 * @file        godot_hud_ground_crumbs.hpp
 * @brief       The dark crumbs of broken-up soil where a loosen verb went into the ground
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD row N08, the mockup's drawGround after the rows (m.html 441 to
 *              444): "broken-up soil: dark crumbs where the hoe or the stick went in". They lie on the ground in
 *              every mode, so TerrainKit draws them with the rows of its ground layer (ground_rows, in the mode
 *              terrain ground_terrain), over the lines as the mockup strokes them.
 *
 *              THE LOOSE GRID. The loosened share of each point of the mockup's ground grid is the frame's: its
 *              columns and rows from the south-west point at the step east and north, one share 0..1 per point
 *              (row * columns + column). While flora.patch.v1 carries no loose soil (N0456) the bridge
 *              VivariumLooseBridge lays it from the taps of the loosen verbs; the crumbs draw whatever the frame
 *              carries.
 *
 *              A POINT OVER THE FLOOR. Each point whose share lies above hud_crumb_floor carries
 *              1 + round(share * hud_crumb_count) ellipses. Each is moved by up to half a step east and north
 *              by two draws of the mockup's rng(row * hud_crumb_seed_row + column), east first; on the ground at
 *              that place it is hud_crumb_rx_share of the drawn step east plus hud_crumb_rx wide and
 *              hud_crumb_ry_share of it plus hud_crumb_ry high, in HUD units, hud_crumb_steps points around. Its
 *              colour is hud_crumb_lit_* times the frame's light plus hud_crumb_dark_* per channel, each floored
 *              to a whole 0..255 as the mockup's |0, its alpha hud_crumb_alpha plus hud_crumb_alpha_loose times
 *              the share. A point whose place or whose step east lies off the ground or behind the camera draws
 *              no crumb.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100389 - level the ground
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0456 (flora.patch.v1 loose: the bridge lays the grid), carrier N0448
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_anchor.hpp>
#include <ase/adp/godot/godot_hud_ui_ground.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** The loosened soil this frame and the light it lies in. */
struct GroundCrumbsFrame {
    const UiAnchor* anchor = nullptr;            // the projection of this frame
    const UiGround* ground = nullptr;            // the ground the crumbs lie on
    const std::vector<double>* loose = nullptr;  // the share 0..1 per point of the loose grid, row * columns + column
    int64_t columns = 0;                         // points of the loose grid east
    int64_t rows = 0;                            // points of the loose grid north
    double west_x = 0.0;                         // the south-west point, world metres east
    double south_z = 0.0;                        // the south-west point, world metres north
    double step_x_m = 0.0;                       // the distance of two points east, metres
    double step_z_m = 0.0;                       // the distance of two points north, metres
    double light = 1.0;                          // the mockup's light(): the sun's height and the overcast shade
};

/** The crumbs of loosened soil on the ground. */
class GroundCrumbs {
public:
    /** Binds the measures; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the crumbs of `now` on a canvas already set to the HUD's frame. */
    void draw(UiPen& canvas, const GroundCrumbsFrame& now) const;

private:
    double floor_ = 0.0;        // hud_crumb_floor
    double count_ = 0.0;        // hud_crumb_count
    double seed_row_ = 0.0;     // hud_crumb_seed_row
    double rx_share_ = 0.0;     // hud_crumb_rx_share
    double rx_ = 0.0;           // hud_crumb_rx
    double ry_share_ = 0.0;     // hud_crumb_ry_share
    double ry_ = 0.0;           // hud_crumb_ry
    double steps_ = 0.0;        // hud_crumb_steps
    double lit_r_ = 0.0, lit_g_ = 0.0, lit_b_ = 0.0;     // hud_crumb_lit_*
    double dark_r_ = 0.0, dark_g_ = 0.0, dark_b_ = 0.0;  // hud_crumb_dark_*
    double alpha_ = 0.0;        // hud_crumb_alpha
    double alpha_loose_ = 0.0;  // hud_crumb_alpha_loose
};

}  // namespace ase::adp::godot
