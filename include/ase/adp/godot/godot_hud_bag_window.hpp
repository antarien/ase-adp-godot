#pragma once

/**
 * ASE GODOT ADAPTER - HUD BAG WINDOW
 *
 * @file        godot_hud_bag_window.hpp
 * @brief       The leather pack and the linen pouch opened: a window each, things where they were let go
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.7, block BagWindow (the mockup's bagWin and pouchWin,
 *              layer 14 "Ledertasche, dann Leinenbeutel", drawn while open). Every measure and colour named
 *              below is read by read_bag_window_look (godot_hud_bag_window_look.hpp).
 *
 *              THE LEATHER PACK (hud_bag_*). A flap rising hud_bag_flap_rise over the body, its seam stitched
 *              hud_bag_stitch_dash / hud_bag_stitch_gap, the title texts.hud.items.ranzen.name on it (class tb,
 *              LEATHER_BAG_TITLE); the body with rounded corners (hud_bag_corner at the top, hud_bag_corner_low
 *              and hud_bag_corner_low_reach at the foot) in LEATHER_BAG_BODY, edged LEATHER_BAG_EDGE; the inner
 *              field inset by hud_bag_field_*, LEATHER_BAG_INNER, stitched LEATHER_BAG_INNER_SEAM; the cross
 *              (texts.hud.window_close, its icon through UiIcon) at the upper right; the things through
 *              BagContents from the content corner; at perception levels[1] the load texts.hud.load at the lower
 *              right (LEATHER_BAG_LOAD), at levels[2] the mouth texts.hud.mouth at the lower left
 *              (LEATHER_BAG_SPEC).
 *
 *              THE LINEN POUCH (hud_pouch_*). A sack drawn from the neck down to a sagging foot in
 *              LINEN_POUCH_BODY, edged LINEN_POUCH_EDGE, a fold down its left side, the drawstring and its two
 *              knotted ends in LINEN_POUCH_STRING, the title texts.hud.items.bolsa.name, the cross; its things
 *              in LINEN_POUCH_INK; at levels[1] texts.hud.load_pouch centred below it (OVERLAY_INK_SOFT). It opens
 *              only while the pouch lies among the carried things, and closes with the pack.
 *
 *              THE GESTURES. A tap on the cross opens or closes; the flap and the neck are the grips that drag
 *              a window, kept inside the frame (hud_window_margin_x at the sides, hud_bag_min_y /
 *              hud_pouch_min_y at the top, hud_window_max_bottom at the foot); the body takes a touch and does
 *              nothing; a thing is hit through BagContents. hit names the part as the mockup's data-act
 *              (GODOT_HUD_ACT_*). over tells the drag layer whether a drag stands over a window: the pouch first,
 *              its box raised by hud_pouch_drop_rise and lowered by hud_pouch_drop_sink, then the pack, raised
 *              by hud_bag_drop_rise.
 *
 *              THE HOT EDGE. While a moved drag stands over a window, its body is edged in DOMAIN_LIFE, or in
 *              DOMAIN_ALERT when the FitCheck refuses the thing, at hud_bag_hot_line / hud_pouch_hot_line.
 *
 *              FULL FROM THE FRAME. Capacity, mouth, load and the things are state of ase-inventory on
 *              flora.carry.v1. The window draws them from ContainerDef::physical() and the frame's things at
 *              every perception level and never asks whether the port delivered: while it delivers none, the
 *              bridge VivariumCarryBridge writes the mockup's CAPS from SHA (hud_bridge_container_<id>_*) and
 *              marks the gap N0451 in its own source.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_bag_contents.hpp>
#include <ase/adp/godot/godot_hud_bag_window_look.hpp>
#include <ase/adp/godot/godot_hud_container_def.hpp>
#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the windows show this frame. */
struct BagWindowFrame {
    uint32_t level = 0;                                 // the depth of perception
    const ContainerDefCarry* pack = nullptr;            // flora.carry.v1: the pack's record, nullptr while none
    const ContainerDefCarry* pouch = nullptr;           // flora.carry.v1: the pouch's record, nullptr while none
    const std::vector<BagThing>* pack_things = nullptr; // the things in the pack, nullptr while none
    const std::vector<BagThing>* pouch_things = nullptr;// the things in the pouch, nullptr while none
    bool pouch_carried = false;                         // the pouch lies among the carried things
    std::string selected;                               // S.sel
    std::string dragged;                                // the item a moved drag holds
    const char* hot = nullptr;                          // GODOT_HUD_CONTAINER_* a moved drag stands over
    bool refused = false;                               // the FitCheck refuses the dragged thing there
};

/** The part of a window a touch landed on. */
struct BagWindowHit {
    const char* part = nullptr;  // GODOT_HUD_ACT_*, nullptr when the touch missed both windows
    std::string key;             // the item, for GODOT_HUD_ACT_ITEM
};

/** The window a drag stands over. */
struct BagWindowDrop {
    const char* container = nullptr;     // GODOT_HUD_CONTAINER_*, nullptr when over neither
    ::godot::Vector2 corner;             // the content corner of that window
    const ContainerDef* def = nullptr;   // its container
};

/** The leather pack and the linen pouch opened. */
class BagWindow {
public:
    /** Binds measures, colours, words, the cross and both containers; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The cross or the bag icon: opens the pack, or closes it and the pouch with it. */
    void toggle_pack();

    /** A double tap on the pouch among the pack's things: opens or closes it. */
    void toggle_pouch();

    /** The pouch's cross. */
    void close_pouch();

    bool pack_open() const { return pack_open_; }
    bool pouch_open() const { return pouch_open_; }

    /** Takes the window of the grip `part` (GODOT_HUD_ACT_PACK_GRIP, GODOT_HUD_ACT_POUCH_GRIP) at `point`. */
    void grab(const char* part, const ::godot::Vector2& point);

    /** Moves the grabbed window with the finger, inside the frame; false when no window is held. */
    bool move(const ::godot::Vector2& point);

    /** Lets the window go; false when none was held. */
    bool release();

    /**
     * Takes this frame's container records of the pack and the pouch from the carry bridge. Runs every frame, drawn
     * or not, so FitCheck, DragLayer and the bridge's edge read the record of this frame even while no window is open.
     */
    void take(const BagWindowFrame& now);

    /** Draws both windows under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const BagWindowFrame& now,
              std::string& failed);

    /** The part under `point` as last drawn, the topmost first. */
    BagWindowHit hit(const ::godot::Vector2& point) const;

    /** The window a drag at `point` stands over, the pouch first. */
    BagWindowDrop over(const ::godot::Vector2& point) const;

    /**
     * The hit boxes of the crosses as last drawn: `pack` closes the pack (GODOT_HUD_ACT_PACK_CLOSE), `pouch` the pouch
     * (GODOT_HUD_ACT_POUCH_CLOSE); each `*_drawn` is false while its window is not drawn. hit() asks the same boxes.
     */
    void close_places(::godot::Rect2& pack, bool& pack_drawn, ::godot::Rect2& pouch, bool& pouch_drawn) const;

    /**
     * The grips as last drawn, for the place lines a test taps: `pack` the part of the flap above the body
     * (GODOT_HUD_ACT_PACK_GRIP, the mockup's data-act pgrip), `pouch` the neck strip (GODOT_HUD_ACT_POUCH_GRIP,
     * pogrip). hit() answers the grip for every point of these boxes the body and the things do not cover.
     */
    void grip_places(::godot::Rect2& pack, bool& pack_drawn, ::godot::Rect2& pouch, bool& pouch_drawn) const;

    /** Where each thing was last drawn, the pack's then the pouch's (the mockup's POS). */
    const std::vector<BagThingPlace>& pack_places() const { return pack_places_; }
    const std::vector<BagThingPlace>& pouch_places() const { return pouch_places_; }

    /** The containers of both windows, read by DragLayer for the scale of a ghost and the free spot of a drop. */
    const ContainerDef& pack_def() const { return pack_def_; }
    const ContainerDef& pouch_def() const { return pouch_def_; }

private:
    bool draw_pack(UiPen& canvas, const ::godot::Transform2D& frame, const BagWindowFrame& now,
                   std::string& failed);
    bool draw_pouch(UiPen& canvas, const ::godot::Transform2D& frame, const BagWindowFrame& now,
                    std::string& failed);

    /** Sets `block` to `word` filled with `values` and draws it at `place`. */
    bool say(UiPen& canvas, const ::godot::Transform2D& frame, UiTextBlock& block,
             const std::vector<HudRun>& word, const std::vector<HudPortValue>& values, const UiTextPlace& place,
             std::string& failed);

    /** A weight or a volume as the mockup prints it: f1, or kgf when `fine`. */
    std::string number(double value, bool fine) const;

    BagWindowLook look_;          // every measure and colour, read_bag_window_look
    uint32_t level_load_ = 0;     // hud.perception.levels[1]: load shown
    uint32_t level_mouth_ = 0;    // hud.perception.levels[2]: mouth shown
    std::vector<HudRun> pack_title_, pouch_title_, load_, load_pouch_, mouth_;
    UiTextBlock title_;           // class tb
    UiTextBlock data_;            // class d
    UiIcon close_;                // texts.hud.window_close
    ContainerDef pack_def_;       // family bag
    ContainerDef pouch_def_;      // family pouch
    BagContents pack_contents_;
    BagContents pouch_contents_;

    ::godot::Vector2 pack_at_;    // S.win.ranzen
    ::godot::Vector2 pouch_at_;   // S.win.bolsa
    bool pack_open_ = false;      // S.bagOpen
    bool pouch_open_ = false;     // S.pouchOpen
    const char* held_ = nullptr;  // S.wdrag.w: the grip held, nullptr when none
    ::godot::Vector2 held_from_;  // S.wdrag.ox, oy

    std::vector<UiSvgOutline> pack_flap_, pack_body_, pouch_body_;  // as last drawn, for hit
    std::vector<BagThingPlace> pack_places_, pouch_places_;
    bool pouch_drawn_ = false;
    bool pack_drawn_ = false;
};

}  // namespace ase::adp::godot
