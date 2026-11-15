#pragma once

/**
 * ASE GODOT ADAPTER - HUD LEATHER PACK
 *
 * @file        godot_hud_leather_pack.hpp
 * @brief       The widget of the leather pack: the container worn at the hip that every carried thing lies in
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, the table of one widget class per device over
 *              UiItemSprite: LeatherPack, the mockup's ContainerItem of the ranzen (worn). It holds one
 *              ItemWidget and learns its item from hud.items, where the entry naming LeatherPack gives the key
 *              and the mode hold. BagIcon draws it closed at the hip, BagWindow open.
 *
 *              WORN, NEVER HELD. The pack is never taken into the hand and never laid down: a tap on it opens or
 *              closes its window (BagWindow, the mockup's bagOpen), and the place it is in is "worn" (the
 *              mockup's where of the ranzen, getragen). Its weight is its own and that of everything in it, as
 *              ase-inventory carries them on flora.carry.v1; the caller sums the things the port reports inside.
 *              What it lets in, the opening of 45 cm, the room and the carry of its ContainerDef, is the
 *              module's verdict through FitCheck (DSGN_067 inventory control, DSGN_080 69.1).
 *
 *              THE STATE COMES FROM THE FRAME. The pack is drawn whole and never asks whether the port
 *              delivered: while flora.carry.v1 is silent the bridge fills the mockup's carry
 *              (VivariumCarryBridge::fill) and VivariumView::open_gap logs the gap N0451.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067, DSGN_080
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_bag_window.hpp>
#include <ase/adp/godot/godot_hud_item_widget.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The leather pack. */
class LeatherPack {
public:
    /** The class name hud.items names this widget with. */
    static constexpr const char* widget() { return "LeatherPack"; }

    /** Binds the pack's item and all it is drawn with; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The pack fitted into the box of `place`. */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiItemSpritePlace& place) const;

    /** The pack unturned with its view box's corner at `corner`, scaled by `k`. */
    void draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& corner,
                     double k, const UiItemSpritePlace& place) const;

    /** The reading of the pack: its size, and its weight with `contents_kg`, what the port reports inside it. */
    bool reading(const ItemWidgetCarry& carry, double contents_kg, std::vector<HudRun>& out,
                 std::string& failed) const;

    /** The tap on the pack: its window opens or closes (the mockup's bagOpen toggle); worn, it never goes to hand. */
    void tap(BagWindow& window) const;

    const ItemWidget& item() const { return item_; }

private:
    ItemWidget item_;
};

}  // namespace ase::adp::godot
