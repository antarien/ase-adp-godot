#pragma once

/**
 * ASE GODOT ADAPTER - HUD WATERING POT
 *
 * @file        godot_hud_watering_pot.hpp
 * @brief       The widget of the clay watering pot: a vessel that puts the hand in the mode water and shows its water
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, the table "je Gerät eine eigene Widget-Klasse über
 *              UiItemSprite": WateringPot, the mockup's VesselItem of the regador. It holds one ItemWidget and
 *              learns its item from hud.items, where the entry naming WateringPot gives the key and the mode
 *              water. As a vessel it draws its water line (item_art.json level, kind line) at the share it
 *              holds, the mockup's `y = 29 - fill / cap * 17`: the level of the place is held_l over capacity_l
 *              as ase-inventory carries them on flora.carry.v1.
 *
 *              THE STATE COMES FROM THE FRAME. The pot draws the water line of the carry it is given and never
 *              asks whether the port delivered: while flora.carry.v1 is silent the bridge fills the mockup's
 *              vessel state (VivariumCarryBridge::fill) and VivariumView::open_gap logs the gap N0451.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.01.00002
 */

#include <ase/adp/godot/godot_hud_item_widget.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The clay watering pot. */
class WateringPot {
public:
    /** The class name hud.items names this widget with. */
    static constexpr const char* widget() { return "WateringPot"; }

    /** Binds the pot's item and all it is drawn with; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The pot fitted into the box of `place` with its water line at what `carry` holds. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiItemSpritePlace& place,
              const ItemWidgetCarry& carry) const;

    /** The pot unturned with its view box's corner at `corner`, scaled by `k`, its water line as in draw. */
    void draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& corner,
                     double k, const UiItemSpritePlace& place, const ItemWidgetCarry& carry) const;

    /** The reading of the pot as ase-inventory carries it (its volume included). */
    bool reading(const ItemWidgetCarry& carry, std::vector<HudRun>& out, std::string& failed) const;

    const ItemWidget& item() const { return item_; }

    /** `place` with the share the pot holds; a pot without capacity draws no line. The bridge takes the level of
     *  every carried vessel from it, so the bag, the belt, the hand and the ghost draw the water line drawn here. */
    static UiItemSpritePlace filled(const UiItemSpritePlace& place, const ItemWidgetCarry& carry);

private:
    ItemWidget item_;
};

}  // namespace ase::adp::godot
