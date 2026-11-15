/**
 * ASE GODOT ADAPTER - HUD KNIFE TOOL
 *
 * @file        godot_hud_knife_tool.cpp
 * @brief       The knife's widget: binds its item by its class name and draws it through ItemWidget
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_knife_tool.hpp. A tool carries no
 *              level and no count; its place in the hand, the belt or the bag is read by the block that holds
 *              it from flora.carry.v1 (N0451).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_knife_tool.hpp>

namespace ase::adp::godot {

bool KnifeTool::bind(const UiTheme& theme, std::string& failed) {
    return item_.bind(theme, widget(), failed);
}

void KnifeTool::draw(UiPen& canvas, const ::godot::Transform2D& frame,
                     const UiItemSpritePlace& place) const {
    item_.draw(canvas, frame, place);
}

void KnifeTool::draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame,
                            const ::godot::Vector2& corner, double k, const UiItemSpritePlace& place) const {
    item_.draw_scaled(canvas, frame, corner, k, place);
}

bool KnifeTool::reading(const ItemWidgetCarry& carry, std::vector<HudRun>& out, std::string& failed) const {
    return item_.reading(carry, false, out, failed);
}

}  // namespace ase::adp::godot
