#pragma once

/**
 * ASE GODOT ADAPTER - HUD ITEM WIDGET
 *
 * @file        godot_hud_item_widget.hpp
 * @brief       The part every item widget class holds: its item, drawing, tone, words, reading and stack count
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, the item widgets of the table of one widget
 *              class per device over UiItemSprite (DibberTool, SpadeTool, WateringPot, ...). Each class holds one
 *              ItemWidget by composition and adds what its kind does: a vessel its level, a stack its count, a
 *              container its load, a ground tool its verbs.
 *
 *              THE ITEM COMES FROM THE FILE. bind names the class (its own name, as a system names itself);
 *              UiTheme::item_widget answers with the item of hud.items that names the class, its mode and its
 *              ground verbs. From the item key every other value follows:
 *                drawing   UiItemSprite of item_art.json under the key
 *                tone      DOMAIN_<hud_item_<id>_tone>
 *                hold      texts.hud.<hud_item_<id>_hold>, none for a tool at work
 *                words     texts.hud.items.<id>.name and .case
 *
 *              THE READING (the mockup's info line, `${D.len} CM · ${kgf} KG · ${l} L · ×${n}`): texts.hud.item_size
 *              with {cm} as written and {kg} to hud_load_decimals (hud_load_fine_decimals below
 *              hud_load_fine_below), then texts.hud.item_volume {l} where the thing holds hud_item_volume_from
 *              litres or more, then texts.hud.item_count {n} for a stack. The values are ase-inventory's.
 *
 *              THE COUNT (the mockup's ×n at the corner of a stack's slot, `x+w+1, y+h+2`, text-anchor end):
 *              texts.hud.stack_count in class d at hud_item_count_size in the ink the place gives,
 *              hud_item_count_dx right of and hud_item_count_dy below the lower right corner of the box, drawn
 *              only for more than one.
 *
 *              THE STATE COMES FROM THE FRAME. A thing's state is ase-inventory's (Entity-per-Item), delivered
 *              on flora.carry.v1 (N0451); a vessel's sap on N0457. The widget draws every value of the carry it
 *              is handed and never asks whether a port delivered it: while a port is silent the bridge fills the
 *              carry with the mockup's values from SHA and VivariumView::open_gap logs the gap once on its edge.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), N0457 (sap), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_item_sprite.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One thing as flora.carry.v1 delivers it: an item entity of ase-inventory and its components. */
struct ItemWidgetCarry {
    double length_cm = 0.0;   // its longest piece
    double mass_kg = 0.0;     // of one
    double volume_l = 0.0;    // of one
    uint32_t count = 0;       // a stack: how many lie together
    double held_l = 0.0;      // a vessel: what it holds now
    double capacity_l = 0.0;  // a vessel: what it holds at most
};

/** The part every item widget class holds. */
class ItemWidget {
public:
    /** Binds the item of the class `widget` and all it is drawn with; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, const char* widget, std::string& failed);

    /** artFit: the thing fitted into the box of `place` (UiItemSprite::draw). */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, const UiItemSpritePlace& place) const;

    /** artSvg: the thing unturned with its view box's corner at `corner`, scaled by `k` (UiItemSprite::draw_scaled). */
    void draw_scaled(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& corner,
                     double k, const UiItemSpritePlace& place) const;

    /** The count ×n at the corner of the box of `place` in `ink`, for more than one; false with the reason in `failed`. */
    bool draw_count(UiPen& canvas, const ::godot::Transform2D& frame, const UiItemSpritePlace& place,
                    uint32_t count, const ::godot::Color& ink, std::string& failed);

    /** The reading of `carry` (item_size, item_volume, item_count of a stack); false with the reason in `failed`. */
    bool reading(const ItemWidgetCarry& carry, bool stack, std::vector<HudRun>& out, std::string& failed) const;

    const std::string& widget() const { return widget_; }
    const std::string& id() const { return id_; }
    const std::string& mode() const { return mode_; }
    const std::vector<std::string>& verbs() const { return verbs_; }
    const ::godot::Color& tone() const { return tone_; }
    const std::vector<HudRun>& name() const { return name_; }
    const std::vector<HudRun>& case_words() const { return case_; }
    const std::vector<HudRun>& hold() const { return hold_; }
    bool holds() const { return !hold_.empty(); }
    const UiItemSprite& sprite() const { return sprite_; }

private:
    /** {kg} as the mockup's kgf writes it. */
    std::string kilograms(double value) const;

    std::string widget_;
    std::string id_;
    std::string mode_;
    std::vector<std::string> verbs_;
    UiItemSprite sprite_;
    ::godot::Color tone_;
    std::vector<HudRun> name_;           // texts.hud.items.<id>.name
    std::vector<HudRun> case_;            // texts.hud.items.<id>.case
    std::vector<HudRun> hold_;            // texts.hud.<hud_item_<id>_hold>, empty for none
    std::vector<HudRun> size_word_;       // texts.hud.item_size
    std::vector<HudRun> volume_word_;     // texts.hud.item_volume
    std::vector<HudRun> count_word_;      // texts.hud.item_count
    std::vector<HudRun> stack_word_;      // texts.hud.stack_count
    UiTextBlock count_text_;              // class d
    double count_dx_ = 0.0, count_dy_ = 0.0, count_size_ = 0.0;  // hud_item_count_*
    double volume_from_ = 0.0;                                     // hud_item_volume_from
    double decimals_ = 0.0, fine_below_ = 0.0, fine_decimals_ = 0.0;  // hud_load_*
};

}  // namespace ase::adp::godot
