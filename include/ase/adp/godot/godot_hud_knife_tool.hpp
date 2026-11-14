#pragma once

/**
 * ASE GODOT ADAPTER - HUD KNIFE TOOL
 *
 * @file        godot_hud_knife_tool.hpp
 * @brief       The widget of the iron knife: a tool held in the hand that has no use in the garden yet
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, the table of one widget class per device over
 *              UiItemSprite: KnifeTool, the mockup's ToolItem of the faca. It holds one ItemWidget and learns
 *              its item from hud.items, where the entry naming KnifeTool gives the key and the mode hold. The
 *              knife names no care case of its own (its case words read "kein Fall in Phase 3"); it is a thing
 *              of the growing interface that the keeper carries, so the file names the case of the inventory
 *              blocks it is drawn in. Its hand reading "SEM USO AQUI" is HandSlot's. It draws the tool fitted
 *              into a box or unturned at a scale, and gives its reading.
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
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/godot_hud_item_widget.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The iron knife. */
class KnifeTool {
public:
    /** The class name hud.items names this widget with. */
    static constexpr const char* widget() { return "KnifeTool"; }

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
