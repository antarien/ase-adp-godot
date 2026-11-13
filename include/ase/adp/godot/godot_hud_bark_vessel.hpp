#pragma once

/**
 * ASE GODOT ADAPTER - HUD BARK VESSEL
 *
 * @file        godot_hud_bark_vessel.hpp
 * @brief       The widget of the birch-bark vessel: the vessel that catches the sap of a tap and shows its fill
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, the table "je Gerät eine eigene Widget-Klasse über
 *              UiItemSprite": BarkVessel, the mockup's VesselItem of the cocho. It holds one ItemWidget and
 *              learns its item from hud.items, where the entry naming BarkVessel gives the key and the mode hold.
 *              As a vessel it draws its sap body (item_art.json level, kind body, fill SAP_HELD) at the share it
 *              holds, the mockup's `y = 22 - 17 * min(1, sap / 1.5)`: the level of the place is held_l over
 *              capacity_l as ase-inventory carries them on flora.carry.v1, and no body is drawn at or below
 *              hud_item_sap_from litres (the mockup's `o.sap > 0.02`). It catches the sap of usecase:101609
 *              (laid for N0468).
 *
 *              THE STATE COMES FROM THE FRAME. The sap a vessel holds is its carry state (N0451); while it hangs
 *              at a running tap the fill grows with the tap of N0457, which FocusCluster and HandMode read. The
 *              vessel draws the sap body of the carry it is given and never asks whether the port delivered:
 *              while flora.carry.v1 is silent the bridge fills the mockup's vessel state
 *              (VivariumCarryBridge::fill) and VivariumView::open_gap logs the gap.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:101609 - tap birch sap
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0451 (flora.carry.v1), N0457 (birch sap), N0468 (case laid), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.01.00002
 */

#include <ase/adp/godot/godot_hud_item_widget.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The birch-bark vessel. */
class BarkVessel {
public:
    /** The class name hud.items names this widget with. */
    static constexpr const char* widget() { return "BarkVessel"; }

    /** Binds the vessel's item, the sap threshold and all it is drawn with; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The vessel fitted into the box of `place` with its sap body at what `carry` holds. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiItemSpritePlace& place,
              const ItemWidgetCarry& carry) const;

    /** The vessel unturned with its view box's corner at `corner`, scaled by `k`, its sap body as in draw. */
    void draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& corner,
                     double k, const UiItemSpritePlace& place, const ItemWidgetCarry& carry) const;

    /** The reading of the vessel as ase-inventory carries it (its volume included). */
    bool reading(const ItemWidgetCarry& carry, std::vector<HudRun>& out, std::string& failed) const;

    const ItemWidget& item() const { return item_; }

    /** `place` with the share of sap the vessel holds, or with no level at or below sap_from_. The bridge takes
     *  the level of every carried vessel from it, so the bag, the belt, the hand and the ghost draw this body. */
    UiItemSpritePlace filled(const UiItemSpritePlace& place, const ItemWidgetCarry& carry) const;

private:
    ItemWidget item_;
    double sap_from_ = 0.0;  // hud_item_sap_from
};

}  // namespace ase::adp::godot
