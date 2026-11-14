#pragma once

/**
 * ASE GODOT ADAPTER - HUD LINEN POUCH
 *
 * @file        godot_hud_linen_pouch.hpp
 * @brief       The widget of the linen pouch: the container of the seed, carried in the leather pack and opened apart
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, the table of one widget class per device over
 *              UiItemSprite: LinenPouch, the mockup's ContainerItem of the bolsa. It holds one ItemWidget and
 *              learns its item from hud.items, where the entry naming LinenPouch gives the key and the mode hold.
 *
 *              A CONTAINER, NOT A TOOL. Its weight is its own and that of everything in it (the mockup's wt, which
 *              adds what lies inside the bolsa); ase-inventory carries both on flora.carry.v1, the caller sums
 *              the things the port reports inside. A double tap opens or closes its window (BagWindow, the
 *              mockup's pouchOpen) instead of taking it into the hand. What it lets in, the opening, room and
 *              carry of its ContainerDef, is the module's verdict through FitCheck.
 *
 *              THE STATE COMES FROM THE FRAME. The pouch is drawn whole and never asks whether the port
 *              delivered: while flora.carry.v1 is silent the bridge fills the mockup's carry
 *              (VivariumCarryBridge::fill) and VivariumView::open_gap logs the gap N0451.
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

#include <ase/adp/godot/godot_hud_bag_window.hpp>
#include <ase/adp/godot/godot_hud_item_widget.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The linen pouch. */
class LinenPouch {
public:
    /** The class name hud.items names this widget with. */
    static constexpr const char* widget() { return "LinenPouch"; }

    /** Binds the pouch's item and all it is drawn with; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The pouch fitted into the box of `place`. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiItemSpritePlace& place) const;

    /** The pouch unturned with its view box's corner at `corner`, scaled by `k`. */
    void draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& corner,
                     double k, const UiItemSpritePlace& place) const;

    /** The reading of the pouch: its size, and its weight with `contents_kg`, what the port reports inside it. */
    bool reading(const ItemWidgetCarry& carry, double contents_kg, std::vector<HudRun>& out,
                 std::string& failed) const;

    /** The double tap on the pouch: its window opens or closes (the mockup's pouchOpen toggle). */
    void double_tap(BagWindow& window) const;

    const ItemWidget& item() const { return item_; }

private:
    ItemWidget item_;
};

}  // namespace ase::adp::godot
