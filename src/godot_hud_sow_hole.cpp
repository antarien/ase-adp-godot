/**
 * ASE GODOT ADAPTER - HUD SOW HOLE
 *
 * @file        godot_hud_sow_hole.cpp
 * @brief       The sow hole's drawing: an ellipse at the projected place while it stays open, all from SHA
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_sow_hole.hpp. The mockup's line
 *              (m.html 477): while performance.now() < S.hole.until, ellipse(sx(u, z), sy(u, z, hAt(u, z)), 2.6,
 *              1.1) in rgba(28, 20, 12, .9); S.hole.until is the sowing plus 1600 ms.
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

#include <ase/adp/godot/godot_hud_sow_hole.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>

#include <godot_cpp/variant/vector2.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool SowHole::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_world_sow_hole_ms", &open_ms_}, {"hud_world_sow_hole_rx", &rx_}, {"hud_world_sow_hole_ry", &ry_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    if (!theme.color("WORLD_DIBBER_HOLE", hole_)) {
        failed = "WORLD_DIBBER_HOLE";
        return false;
    }
    return true;
}

void SowHole::draw(UiPen& canvas, const ::godot::Transform2D& frame, const SowHoleFrame& now) const {
    // THE PLANTING HOLE THE SPADE DUG stands until amontoar closes it (operator 2026-11-13, usecase:100388, Q02 to
    // Q04); the dibber's hole of sowTap closes after hud_world_sow_hole_ms on its own.
    const bool dibbed = now.sown && now.age >= 0.0 && now.age * 1000.0 < open_ms_;
    if (now.anchor == nullptr || (!now.open && !dibbed)) {
        return;  // no hole is dug, no seed went in, or the seed has closed its hole
    }
    ::godot::Vector2 at;
    if (!now.anchor->place(now.place, at)) {
        return;  // the place lies behind the camera
    }
    const ::godot::Transform2D oval(0.0f, ::godot::Size2(static_cast<float>(rx_), static_cast<float>(ry_)), 0.0f, at);
    canvas.draw_set_transform_matrix(frame * oval);
    canvas.draw_circle(::godot::Vector2(), 1.0f, hole_);
    canvas.draw_set_transform_matrix(::godot::Transform2D());
}

}  // namespace ase::adp::godot
