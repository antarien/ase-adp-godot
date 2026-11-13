#pragma once

/**
 * ASE GODOT ADAPTER - HUD RAKE TOOL
 *
 * @file        godot_hud_rake_tool.hpp
 * @brief       The widget of the rake: a ground tool that puts the hand in the mode terrain
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, the table "je Gerät eine eigene Widget-Klasse über
 *              UiItemSprite": RakeTool, the mockup's ToolItem of the ancinho. It holds one ItemWidget and learns
 *              its item from hud.items, where the entry naming RakeTool gives the key, the mode terrain and the
 *              ground verb (aplanar) that TerrainKit offers while it is in the hand. It draws the tool fitted
 *              into a box or unturned at a scale, and gives its reading. The case is the one
 *              hud.items.ancinho.case names.
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
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_hud_item_widget.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The rake. */
class RakeTool {
public:
    /** The class name hud.items names this widget with. */
    static constexpr const char* widget() { return "RakeTool"; }

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
