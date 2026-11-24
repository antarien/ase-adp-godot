/**
 * ASE GODOT ADAPTER - HUD WATERING POT
 *
 * @file        godot_hud_watering_pot.cpp
 * @brief       The watering pot's widget: its water line from the vessel state of the frame
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_watering_pot.hpp. The share is
 *              held_l over capacity_l of the carry the bridge fills; an empty pot (the mockup's fill 0) draws
 *              no line.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
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

#include <ase/adp/godot/godot_hud_watering_pot.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>

#include <ase/math/math.hpp>

namespace ase::adp::godot {

bool WateringPot::bind(const UiTheme& theme, std::string& failed) {
    return item_.bind(theme, widget(), failed);
}

UiItemSpritePlace WateringPot::filled(const UiItemSpritePlace& place, const ItemWidgetCarry& carry) {
    UiItemSpritePlace out = place;
    out.level = 0.0;
    if (carry.capacity_l > 0.0) {
        out.level = ase::math::clamp(carry.held_l / carry.capacity_l, 0.0, 1.0);
    }
    return out;
}

void WateringPot::draw(UiPen& canvas, const ::godot::Transform2D& frame,
                       const UiItemSpritePlace& place, const ItemWidgetCarry& carry) const {
    item_.draw(canvas, frame, filled(place, carry));
}

void WateringPot::draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame,
                              const ::godot::Vector2& corner, double k, const UiItemSpritePlace& place,
                              const ItemWidgetCarry& carry) const {
    item_.draw_scaled(canvas, frame, corner, k, filled(place, carry));
}

bool WateringPot::reading(const ItemWidgetCarry& carry, std::vector<HudRun>& out, std::string& failed) const {
    return item_.reading(carry, false, out, failed);
}

}  // namespace ase::adp::godot
