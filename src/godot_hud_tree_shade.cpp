/**
 * ASE GODOT ADAPTER - HUD TREE SHADE
 *
 * @file        godot_hud_tree_shade.cpp
 * @brief       The tree shade's drawing: one ellipse at the foot by the stage scale, or the mound and the seed
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_tree_shade.hpp. The mockup's lines
 *              (m.html 488 to 490): at stage 0 ellipse(TX, by, 9, 3) in rgba(70, 52, 30, .9) and ellipse(TX, by - 1,
 *              2, 1.4) in rgba(205, 180, 130, .95), and nothing else of the tree; from stage 1 on
 *              ellipse(TX + 10, by + 2, 46 * sc + 6, 6 * sc + 2) in rgba(0, 0, 0, .22), sc = SCALE[S.stage].
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

#include <ase/adp/godot/godot_hud_tree_shade.hpp>
#include <ase/adp/godot/patch/types.hpp>

#include <godot_cpp/variant/vector2.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool TreeShade::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"plant_shade_dx", &shade_dx_},
             {"plant_shade_dy", &shade_dy_},
             {"plant_shade_rx", &shade_rx_},
             {"plant_shade_rx_base", &shade_rx_base_},
             {"plant_shade_ry", &shade_ry_},
             {"plant_shade_ry_base", &shade_ry_base_},
             {"plant_scale_seed", &scale_seed_},
             {"plant_scale_shoot", &scale_shoot_},
             {"plant_scale_growing", &scale_growing_},
             {"plant_scale_mature", &scale_mature_},
             {"plant_seed_mound_rx", &mound_rx_},
             {"plant_seed_mound_ry", &mound_ry_},
             {"plant_seed_rx", &seed_rx_},
             {"plant_seed_ry", &seed_ry_},
             {"plant_seed_dy", &seed_dy_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{
             {"PLANT_SHADE", &shade_}, {"PLANT_SEED_MOUND", &mound_}, {"PLANT_SEED", &seed_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    return true;
}

void TreeShade::oval(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& at, double rx,
                     double ry, const ::godot::Color& colour) {
    const ::godot::Transform2D shape(0.0f, ::godot::Size2(static_cast<float>(rx), static_cast<float>(ry)), 0.0f, at);
    canvas.draw_set_transform_matrix(frame * shape);
    canvas.draw_circle(::godot::Vector2(), 1.0f, colour);
}

void TreeShade::draw(UiPen& canvas, const ::godot::Transform2D& frame, const TreeShadeFrame& now) const {
    if (!now.placed || now.stage >= GODOT_STAGE_COUNT) {
        return;  // the birch stands off the screen, or no patch record names its stage
    }
    if (now.seed) {
        // THE SEED SHOWS ONLY IN THE OPEN HOLE (operator 2026-11-13, before the mockup's m.html 488, which draws the
        // mound and the seed on the soil at stage 0): a seed the keeper laid in the hole lies there visible, a
        // covered hole leaves the mound alone, and a stand nobody sowed leaves the soil bare.
        if (now.covered) {
            oval(canvas, frame, now.trunk, mound_rx_, mound_ry_, mound_);
        } else if (now.seeded) {
            oval(canvas, frame, now.trunk - ::godot::Vector2(0.0f, static_cast<float>(seed_dy_)), seed_rx_, seed_ry_,
                 seed_);
        }
        canvas.draw_set_transform_matrix(::godot::Transform2D());
        return;
    }
    double scale = scale_mature_;
    if (now.sprout) {
        scale = scale_shoot_;
    } else if (now.grow) {
        scale = scale_growing_;
    }
    const ::godot::Vector2 at =
        now.trunk + ::godot::Vector2(static_cast<float>(shade_dx_), static_cast<float>(shade_dy_));
    oval(canvas, frame, at, shade_rx_ * scale + shade_rx_base_, shade_ry_ * scale + shade_ry_base_, shade_);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
}

}  // namespace ase::adp::godot
