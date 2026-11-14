#pragma once

/**
 * ASE GODOT ADAPTER - HUD DIBBER TOOL
 *
 * @file        godot_hud_dibber_tool.hpp
 * @brief       The widget of the dibber: the tool that puts the hand in the mode sow
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, the table of one widget class per device over
 *              UiItemSprite: DibberTool, the mockup's ToolItem of the plantador. It holds one ItemWidget and
 *              learns its item from hud.items, where the entry naming DibberTool gives the key, the mode the
 *              tool puts the hand in and its ground verbs (none: sowing is the mode, not a ground verb). It
 *              draws the dibber fitted into a box or unturned at a scale, and gives its reading.
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
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_hud_item_widget.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The dibber. */
class DibberTool {
public:
    /** The class name hud.items names this widget with. */
    static constexpr const char* widget() { return "DibberTool"; }

    /** Binds the dibber's item and all it is drawn with; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The dibber fitted into the box of `place`. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiItemSpritePlace& place) const;

    /** The dibber unturned with its view box's corner at `corner`, scaled by `k`. */
    void draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& corner,
                     double k, const UiItemSpritePlace& place) const;

    /** The reading of the dibber as ase-inventory carries it. */
    bool reading(const ItemWidgetCarry& carry, std::vector<HudRun>& out, std::string& failed) const;

    const ItemWidget& item() const { return item_; }

private:
    ItemWidget item_;
};

}  // namespace ase::adp::godot
