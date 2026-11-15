/**
 * ASE GODOT ADAPTER - HUD BARK VESSEL
 *
 * @file        godot_hud_bark_vessel.cpp
 * @brief       The bark vessel's widget: its sap body from the vessel state of the frame
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_bark_vessel.hpp. The share is
 *              held_l over capacity_l of the carry the bridge fills; a vessel holding hud_item_sap_from litres
 *              or less draws no body, as the mockup's `o.sap > 0.02`.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:101609 - tap birch sap
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0451 (flora.carry.v1), N0457 (birch sap), N0468 (case laid), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_bark_vessel.hpp>

#include <ase/math/math.hpp>

namespace ase::adp::godot {

bool BarkVessel::bind(const UiTheme& theme, std::string& failed) {
    if (!theme.measure("hud_item_sap_from", sap_from_)) {
        failed = "hud_item_sap_from";
        return false;
    }
    return item_.bind(theme, widget(), failed);
}

UiItemSpritePlace BarkVessel::filled(const UiItemSpritePlace& place, const ItemWidgetCarry& carry) const {
    UiItemSpritePlace out = place;
    out.level = 0.0;
    if (carry.capacity_l > 0.0 && carry.held_l > sap_from_) {
        out.level = ase::math::clamp(carry.held_l / carry.capacity_l, 0.0, 1.0);
    }
    return out;
}

void BarkVessel::draw(UiPen& canvas, const ::godot::Transform2D& frame,
                      const UiItemSpritePlace& place, const ItemWidgetCarry& carry) const {
    item_.draw(canvas, frame, filled(place, carry));
}

void BarkVessel::draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame,
                             const ::godot::Vector2& corner, double k, const UiItemSpritePlace& place,
                             const ItemWidgetCarry& carry) const {
    item_.draw_scaled(canvas, frame, corner, k, filled(place, carry));
}

bool BarkVessel::reading(const ItemWidgetCarry& carry, std::vector<HudRun>& out, std::string& failed) const {
    return item_.reading(carry, false, out, failed);
}

}  // namespace ase::adp::godot
