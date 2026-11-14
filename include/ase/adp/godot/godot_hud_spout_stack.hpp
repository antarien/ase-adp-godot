#pragma once

/**
 * ASE GODOT ADAPTER - HUD SPOUT STACK
 *
 * @file        godot_hud_spout_stack.hpp
 * @brief       The widget of the elder spouts: a stack of hollow twigs, held, that the tap of the mode sap uses
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, the table of one widget class per device over
 *              UiItemSprite: SpoutStack, the mockup's StackItem of the bica. It holds one ItemWidget and learns
 *              its item from hud.items, where the entry naming SpoutStack gives the key and the mode hold. As a
 *              stack it draws its count at the lower right of its box (`stack_count`, the mockup's ×n at
 *              `(x+w+1, y+h+2)`) while it holds more than one, and its reading names the count. The count is the
 *              one ase-inventory carries on flora.carry.v1. A tap spends one spout (usecase:101609, laid for
 *              N0468).
 *
 *              THE STATE COMES FROM THE FRAME. The stack draws the count of the carry it is given and never asks
 *              whether the port delivered: while flora.carry.v1 is silent the bridge fills the mockup's count
 *              (VivariumCarryBridge::fill) and VivariumView::open_gap logs the gap N0451.
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

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** The stack of elder spouts. */
class SpoutStack {
public:
    /** The class name hud.items names this widget with. */
    static constexpr const char* widget() { return "SpoutStack"; }

    /** Binds the stack's item and all it is drawn with; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The stack fitted into the box of `place` with its count in `ink`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiItemSpritePlace& place,
              const ItemWidgetCarry& carry, const ::godot::Color& ink, std::string& failed);

    /** The stack unturned with its view box's corner at `corner`, scaled by `k`, without its count. */
    void draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& corner,
                     double k, const UiItemSpritePlace& place) const;

    /** The count beside a box the caller laid itself (the hand's own place of the count). */
    bool draw_count(UiPen& canvas, const ::godot::Transform2D& frame, const UiItemSpritePlace& place,
                    uint32_t count, const ::godot::Color& ink, std::string& failed);

    /** The reading of the stack as ase-inventory carries it, its count included. */
    bool reading(const ItemWidgetCarry& carry, std::vector<HudRun>& out, std::string& failed) const;

    const ItemWidget& item() const { return item_; }

private:
    ItemWidget item_;
};

}  // namespace ase::adp::godot
