/**
 * ASE GODOT ADAPTER - HUD SPOUT STACK
 *
 * @file        godot_hud_spout_stack.cpp
 * @brief       The spout stack's widget: its drawing and its count from the carry state of the frame
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_spout_stack.hpp. The count is the
 *              count of the carry the bridge fills; ItemWidget::draw_count draws it only above one, as the
 *              mockup does.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:101609 - tap birch sap
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0451 (flora.carry.v1), N0468 (case laid), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_spout_stack.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>

namespace ase::adp::godot {

bool SpoutStack::bind(const UiTheme& theme, std::string& failed) {
    return item_.bind(theme, widget(), failed);
}

bool SpoutStack::draw(UiPen& canvas, const ::godot::Transform2D& frame,
                      const UiItemSpritePlace& place, const ItemWidgetCarry& carry,
                      const ::godot::Color& ink, std::string& failed) {
    item_.draw(canvas, frame, place);
    return item_.draw_count(canvas, frame, place, carry.count, ink, failed);
}

void SpoutStack::draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame,
                             const ::godot::Vector2& corner, double k, const UiItemSpritePlace& place) const {
    item_.draw_scaled(canvas, frame, corner, k, place);
}

bool SpoutStack::draw_count(UiPen& canvas, const ::godot::Transform2D& frame,
                            const UiItemSpritePlace& place, uint32_t count, const ::godot::Color& ink,
                            std::string& failed) {
    return item_.draw_count(canvas, frame, place, count, ink, failed);
}

bool SpoutStack::reading(const ItemWidgetCarry& carry, std::vector<HudRun>& out, std::string& failed) const {
    return item_.reading(carry, true, out, failed);
}

}  // namespace ase::adp::godot
