#pragma once

/**
 * ASE GODOT ADAPTER - HUD BELT HOLSTER
 *
 * @file        godot_hud_belt_holster.hpp
 * @brief       The leather belt across the foot of the screen and its two holsters, one tool each
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, block BeltHolster (the mockup's belt strap and BELT in
 *              hand(), layer 13 with HandSlot and the bag).
 *
 *              THE STRAP. The path hud_belt_path (SVG path data of theme.css, read through read_svg_path) stroked
 *              in BELT_STRAP at hud_belt_line, then once more in BELT_STRAP_SHADE at hud_belt_stitch_line,
 *              dashed hud_belt_stitch_dash / hud_belt_stitch_gap.
 *
 *              THE HOLSTERS. Two squares of side twice hud_belt_holster_half with corner hud_belt_holster_radius
 *              at (hud_belt_1_x, hud_belt_1_y) and (hud_belt_2_x, hud_belt_2_y), filled HOLSTER_GROUND at
 *              hud_belt_holster_fill_alpha. A holster that holds a thing is edged HOLSTER_EDGE and draws the thing
 *              fitted into hud_belt_art (UiItemSprite, the mockup's artFit), except while that thing is dragged;
 *              an empty one is edged OVERLAY_INK_FAINT, dashed hud_belt_empty_dash / hud_belt_empty_gap. While a
 *              moved drag stands over a holster, its edge is DOMAIN_LIFE, or DOMAIN_ALERT when the FitCheck refuses
 *              the thing, at hud_belt_holster_hot_line.
 *
 *              THE PLACES. A held thing's place is the box of side hud_belt_box about the holster's middle, its
 *              scale hud_belt_box over the drawing's longer side (the mockup's POS), for the drag layer to lift it.
 *
 *              THE GESTURES. hit names a held thing (GODOT_HUD_ACT_ITEM with its key) or an empty holster
 *              (GODOT_HUD_ACT_NOOP); over names the holster within hud_drop_belt_radius of a drag.
 *
 *              BRIDGE GAP N0451. What a holster holds is state of ase-inventory on flora.carry.v1. The belt draws
 *              both holsters in full from BeltHolsterFrame and never asks whether the port delivered: while it
 *              delivers nothing VivariumCarryBridge fills the frame with the mockup's tools, and VivariumView logs
 *              the gap once on its edge (design decision 2026-11-10).
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
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_bag_contents.hpp>
#include <ase/adp/godot/godot_hud_ui_item_sprite.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>
#include <utility>
#include <vector>

namespace ase::adp::godot {

/** What the belt shows this frame. */
struct BeltHolsterFrame {
    const BagThing* first = nullptr;    // the thing in the left holster, nullptr while it is empty
    const BagThing* second = nullptr;   // the thing in the right holster, nullptr while it is empty
    std::string dragged;                // the item a moved drag holds
    const char* hot = nullptr;          // GODOT_HUD_CONTAINER_BELT_* a moved drag stands over
    bool refused = false;               // the FitCheck refuses the dragged thing there
};

/** The part of the belt a touch landed on. */
struct BeltHolsterHit {
    const char* part = nullptr;  // GODOT_HUD_ACT_ITEM or GODOT_HUD_ACT_NOOP, nullptr when the touch missed
    std::string key;             // the item, for GODOT_HUD_ACT_ITEM
};

/** The belt and its two holsters. */
class BeltHolster {
public:
    /** Binds measures, colours, the strap, the word and the sprite tokens; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws strap and holsters under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const BeltHolsterFrame& now,
              std::string& failed);

    /** The holster under `point` as last drawn. */
    BeltHolsterHit hit(const ::godot::Vector2& point) const;

    /** GODOT_HUD_CONTAINER_BELT_* within hud_drop_belt_radius of a drag at `point`, nullptr when none. */
    const char* over(const ::godot::Vector2& point) const;

    /** Where each held thing was last drawn (the mockup's POS). */
    const std::vector<BagThingPlace>& places() const { return places_; }

    /**
     * Each holster's square as last drawn, keyed GODOT_HUD_CONTAINER_BELT_*, held or empty: the places a drag
     * can be carried to (over()); none before the first draw.
     */
    void holster_places(std::vector<std::pair<std::string, ::godot::Rect2>>& out) const;

private:
    /** Draws one holster at `middle` holding `thing` (nullptr when empty). */
    bool draw_holster(UiPen& canvas, const ::godot::Transform2D& frame, const ::godot::Vector2& middle,
                      const char* name, const BagThing* thing, const BeltHolsterFrame& now, std::string& failed);

    /** The drawing of `art`, read once. */
    const UiItemSprite* sprite(const std::string& art, std::string& failed);

    const UiTheme* theme_ = nullptr;
    std::vector<UiSvgOutline> strap_;  // hud_belt_path
    double strap_line_ = 0.0;          // hud_belt_line
    double stitch_line_ = 0.0;         // hud_belt_stitch_line
    double stitch_dash_ = 0.0;         // hud_belt_stitch_dash
    double stitch_gap_ = 0.0;          // hud_belt_stitch_gap
    ::godot::Vector2 at_[2];           // hud_belt_1_*, hud_belt_2_*
    double half_ = 0.0;                // hud_belt_holster_half
    double radius_ = 0.0;              // hud_belt_holster_radius
    double line_ = 0.0;                // hud_belt_holster_line
    double hot_line_ = 0.0;            // hud_belt_holster_hot_line
    double empty_dash_ = 0.0;          // hud_belt_empty_dash
    double empty_gap_ = 0.0;           // hud_belt_empty_gap
    double art_ = 0.0;                 // hud_belt_art
    double box_ = 0.0;                 // hud_belt_box
    double drop_radius_ = 0.0;         // hud_drop_belt_radius
    ::godot::Color strap_ink_, shade_ink_, ground_, edge_, empty_, life_, alert_;
    std::vector<std::pair<std::string, UiItemSprite>> sprites_;

    std::vector<BagThingPlace> places_;
    std::string held_[2];               // the key in each holster as last drawn, empty when none
    bool drawn_ = false;
};

}  // namespace ase::adp::godot
