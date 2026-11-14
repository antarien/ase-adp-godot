#pragma once

/**
 * ASE GODOT ADAPTER - HUD AUGER TOOL
 *
 * @file        godot_hud_auger_tool.hpp
 * @brief       The widget of the spoon auger: the tool that puts the hand in the mode sap
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, the table of one widget class per device over
 *              UiItemSprite: AugerTool, the mockup's ToolItem of the trado. It holds one ItemWidget and learns
 *              its item from hud.items, where the entry naming AugerTool gives the key and the mode sap: the
 *              keeper bores the birch at the trunk to tap its sap (usecase:101609, laid for N0468). What the
 *              mode sap shows at the trunk, the open tap and the fill of the bark vessel, is HandMode's and
 *              FocusCluster's and waits on N0457. It draws the tool fitted into a box or unturned at a scale,
 *              and gives its reading.
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
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_hud_item_widget.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The spoon auger. */
class AugerTool {
public:
    /** The class name hud.items names this widget with. */
    static constexpr const char* widget() { return "AugerTool"; }

    /** Binds the tool's item and all it is drawn with; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The tool fitted into the box of `place`. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiItemSpritePlace& place) const;

    /** The tool unturned with its view box's corner at `corner`, scaled by `k`. */
    void draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& corner,
                     double k, const UiItemSpritePlace& place) const;

    /** The reading of the tool as ase-inventory carries it. */
    bool reading(const ItemWidgetCarry& carry, std::vector<HudRun>& out, std::string& failed) const;

    const ItemWidget& item() const { return item_; }

private:
    ItemWidget item_;
};

}  // namespace ase::adp::godot
