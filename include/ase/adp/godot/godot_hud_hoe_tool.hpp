#pragma once

/**
 * ASE GODOT ADAPTER - HUD HOE TOOL
 *
 * @file        godot_hud_hoe_tool.hpp
 * @brief       The widget of the hoe: a ground tool that puts the hand in the mode terrain
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, the table of one widget class per device over
 *              UiItemSprite: HoeTool, the mockup's ToolItem of the enxada. It holds one ItemWidget and learns
 *              its item from hud.items, where the entry naming HoeTool gives the key, the mode terrain and the
 *              ground verbs (soltar, amontoar) that TerrainKit offers while it is in the hand. It draws the
 *              tool fitted into a box or unturned at a scale, and gives its reading. hud.items.enxada.case
 *              proposes usecase:100401 soak the soil, the soil's case of the Groundwater Simulator; the hand's
 *              case is that of the mode terrain, the one TerrainKit carries.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100389 - level the ground
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

#include <ase/adp/godot/godot_hud_item_widget.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The hoe. */
class HoeTool {
public:
    /** The class name hud.items names this widget with. */
    static constexpr const char* widget() { return "HoeTool"; }

    /** Binds the tool's item and all it is drawn with; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The tool fitted into the box of `place`. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiItemSpritePlace& place) const;

    /** The tool unturned with its view box's corner at `corner`, scaled by `k`. */
    void draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& corner,
                     double k, const UiItemSpritePlace& place) const;

    /** The reading of the tool as ase-inventory carries it. */
    bool reading(const ItemWidgetCarry& carry, std::vector<HudRun>& out, std::string& failed) const;

    /** The ground verbs of hud.items, in the order TerrainKit lays them. */
    const std::vector<std::string>& verbs() const { return item_.verbs(); }

    const ItemWidget& item() const { return item_; }

private:
    ItemWidget item_;
};

}  // namespace ase::adp::godot
