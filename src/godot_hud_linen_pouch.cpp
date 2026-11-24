/**
 * ASE GODOT ADAPTER - HUD LINEN POUCH
 *
 * @file        godot_hud_linen_pouch.cpp
 * @brief       The linen pouch's widget: its drawing, its weight with what it holds, its double tap
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_linen_pouch.hpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100388 - sow a new seed
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_linen_pouch.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>

namespace ase::adp::godot {

bool LinenPouch::bind(const UiTheme& theme, std::string& failed) {
    return item_.bind(theme, widget(), failed);
}

void LinenPouch::draw(UiPen& canvas, const ::godot::Transform2D& frame,
                      const UiItemSpritePlace& place) const {
    item_.draw(canvas, frame, place);
}

void LinenPouch::draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame,
                             const ::godot::Vector2& corner, double k, const UiItemSpritePlace& place) const {
    item_.draw_scaled(canvas, frame, corner, k, place);
}

bool LinenPouch::reading(const ItemWidgetCarry& carry, double contents_kg, std::vector<HudRun>& out,
                         std::string& failed) const {
    // The mockup's wt of the bolsa: its own weight and the weight of everything inside it.
    ItemWidgetCarry full = carry;
    full.mass_kg = carry.mass_kg + contents_kg;
    return item_.reading(full, false, out, failed);
}

void LinenPouch::double_tap(BagWindow& window) const {
    window.toggle_pouch();
}

}  // namespace ase::adp::godot
