/**
 * ASE GODOT ADAPTER - HUD LEVEL TOOL
 *
 * @file        godot_hud_level_tool.cpp
 * @brief       The level's widget: binds its item by its class name and draws it through ItemWidget
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, see godot_hud_level_tool.hpp. A ground tool carries no
 *              level and no count; what it does to the ground is TerrainKit's, its place in the hand, the belt
 *              or the rack is read by the block that holds it from flora.carry.v1 (N0451).
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

#include <ase/adp/godot/godot_hud_level_tool.hpp>

namespace ase::adp::godot {

bool LevelTool::bind(const UiTheme& theme, std::string& failed) {
    return item_.bind(theme, widget(), failed);
}

void LevelTool::draw(UiPen& canvas, const ::godot::Transform2D& frame,
                     const UiItemSpritePlace& place) const {
    item_.draw(canvas, frame, place);
}

void LevelTool::draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame,
                            const ::godot::Vector2& corner, double k, const UiItemSpritePlace& place) const {
    item_.draw_scaled(canvas, frame, corner, k, place);
}

bool LevelTool::reading(const ItemWidgetCarry& carry, std::vector<HudRun>& out, std::string& failed) const {
    return item_.reading(carry, false, out, failed);
}

}  // namespace ase::adp::godot
