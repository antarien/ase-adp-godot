#pragma once

/**
 * ASE GODOT ADAPTER - HUD TREE SHADE
 *
 * @file        godot_hud_tree_shade.hpp
 * @brief       The shade under the birch, or the seed mound while the stand is still a seed
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, element N19 (the mockup's drawTree, m.html 488 to 490).
 *              It stands before the world group, as drawTree before drawWorld, and outside its brightness: the
 *              mockup's filter lies on drawWorld only.
 *
 *              THE DRAWING. From the stage SPROUT on, an ellipse at the foot of the trunk moved by plant_shade_dx
 *              and plant_shade_dy, its radii plant_shade_rx times the stage scale plus plant_shade_rx_base and
 *              plant_shade_ry times the scale plus plant_shade_ry_base, in PLANT_SHADE. The stage scale is
 *              plant_scale_seed, _shoot, _growing or _mature (the mockup's SCALE[S.stage]). At the stage SEED the
 *              mockup draws no shade but the mound of the sown soil: an ellipse plant_seed_mound_rx by _ry in
 *              PLANT_SEED_MOUND at the foot, and the seed plant_seed_rx by _ry in PLANT_SEED, plant_seed_dy above it.
 *
 *              FULL FROM THE FRAME. The stem place and the stage come from the frame the view fills (the stand of
 *              ase-flora on its patch record); TreeShade draws whatever the frame carries and asks no port. The view
 *              sets the stem place on the ground at the stem's x and z as the terrain lies now (UiGround::surface),
 *              so the seed lies on the floor of the planting hole, however deep it was dug. A birch off the screen,
 *              a stem place without ground samples or a patch without a stage draws nothing.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    carrier N0448
 * @created     2026-11-12
 * @modified    2026-11-19
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

/** What the shade under the birch shows this frame. */
struct TreeShadeFrame {
    bool placed = false;          // the birch stands on the screen
    ::godot::Vector2 trunk;       // the stem place on the ground as it lies now, mockup units (TX, TY())
    uint8_t stage = UINT8_MAX;    // the position of the stand's stage in the stage record (VivariumPatchView::stage), UINT8_MAX without a patch record
    bool seed = false;            // the stand is a seed in the soil (VivariumPatchView::seed, usecase:101635)
    bool sprout = false;          // the stand is a seedling (VivariumPatchView::sprout)
    bool grow = false;            // the stand is growing (VivariumPatchView::grow); a placed stand with a stage and none of the three is mature
    bool seeded = false;          // a seed lies in the open planting hole (SowHoleFrame::seeded, Q03)
    bool covered = false;         // the planting hole is closed over its seed (SowHoleFrame::covered, Q04)
};

/** The shade under the birch and the seed mound. */
class TreeShade {
public:
    /** Binds the measures and the colours; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the shade or the mound of `now` under `frame`. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const TreeShadeFrame& now) const;

private:
    /** One ellipse of radii `rx`, `ry` at `at` in `colour` under `frame`. */
    static void oval(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& at, double rx,
                     double ry, const ::godot::Color& colour);

    double shade_dx_ = 0.0;       // plant_shade_dx
    double shade_dy_ = 0.0;       // plant_shade_dy
    double shade_rx_ = 0.0;       // plant_shade_rx
    double shade_rx_base_ = 0.0;  // plant_shade_rx_base
    double shade_ry_ = 0.0;       // plant_shade_ry
    double shade_ry_base_ = 0.0;  // plant_shade_ry_base
    double scale_seed_ = 0.0;     // plant_scale_seed
    double scale_shoot_ = 0.0;    // plant_scale_shoot
    double scale_growing_ = 0.0;  // plant_scale_growing
    double scale_mature_ = 0.0;   // plant_scale_mature
    double mound_rx_ = 0.0;       // plant_seed_mound_rx
    double mound_ry_ = 0.0;       // plant_seed_mound_ry
    double seed_rx_ = 0.0;        // plant_seed_rx
    double seed_ry_ = 0.0;        // plant_seed_ry
    double seed_dy_ = 0.0;        // plant_seed_dy
    ::godot::Color shade_;        // PLANT_SHADE
    ::godot::Color mound_;        // PLANT_SEED_MOUND
    ::godot::Color seed_;         // PLANT_SEED
};

}  // namespace ase::adp::godot
