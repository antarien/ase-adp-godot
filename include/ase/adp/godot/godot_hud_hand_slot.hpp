#pragma once

/**
 * ASE GODOT ADAPTER - HUD HAND SLOT
 *
 * @file        godot_hud_hand_slot.hpp
 * @brief       The hand at the foot of the view: the thing in it, its name, what to do with it and one reading
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block HandSlot, the mockup's hand() without the belt,
 *              the bag and the toast (BeltHolster, BagIcon, Toast are units of their own).
 *
 *              Two lines of OVERLAY_INK_FAINT at hud_hand_divider_y, hud_hand_divider_left_from .. _to and
 *              _right_from .. _to, width hud_hand_divider_line, frame the reading row.
 *
 *              A THING IN THE HAND (and not lifted by a drag): a hexagon of hud_hand_radius at hud_hand_x, _y,
 *              filled in the item's tone (DOMAIN_<hud_item_<id>_tone>) at hud_hand_fill_alpha, outlined in the
 *              tone at hud_hand_line, or DOMAIN_LIFE at hud_hand_hot_line while a drag stands over it (DOMAIN_ALERT
 *              when the hand refuses it); the drawing fitted into hud_hand_art, a vessel with its level. Its place
 *              for the drag layer is the box hud_hand_box round the centre. A stacking thing with more than one
 *              in the hand shows texts.hud.stack_count hud_hand_count_dx right of and below the centre (class d,
 *              hud_hand_count_size, the tone). The stow button: the glyph stow_glyph at hud_hand_put_away_x, _y,
 *              hud_hand_put_away_size in OVERLAY_INK_SOFT, under it texts.hud.stow at _label_y, _label_size in
 *              OVERLAY_INK_FAINT, its hit box hud_hand_put_away_hit_*. Over the row: the item's name (class tb,
 *              hud_hand_title_*, the tone), the instruction HandMode gives (hud_hand_hint_line_*, OVERLAY_INK_SOFT),
 *              from hud.perception.levels[2] the case (class d, hud_hand_case_*, OVERLAY_INK_FAINT); at the right,
 *              end-anchored at hud_hand_reading_x, the reading's label (hud_hand_reading_label_*, OVERLAY_INK_SOFT)
 *              and value (class d, hud_hand_reading_value_*, the tone). A tap on the hand pours in the mode water,
 *              else takes the thing; a tap on a verb chip chooses the verb (HandMode).
 *
 *              THE EMPTY HAND: the hexagon filled in OVERLAY_HAND_EMPTY at hud_hand_empty_fill_alpha, outlined in
 *              OVERLAY_INK_FAINT dashed hud_hand_empty_dash on, _gap off at hud_hand_empty_line, or solid at
 *              hud_hand_hot_line in the hot colour while a drag stands over it; the glyph hand_glyph at
 *              hud_hand_empty_glyph_size in the same colour; texts.hud.hand_free hud_hand_empty_label_dy under the
 *              centre (hud_hand_empty_label_size, OVERLAY_INK_FAINT); in the mode rest at levels[0] before any
 *              toast texts.hud.hint_rest at hud_hand_hint_y (hud_hand_hint_size, OVERLAY_INK_FAINT). The empty
 *              hand takes no tap.
 *
 *              THE PICTURE IS WHOLE. The slot draws what its frame holds and never asks whether a port delivered:
 *              while flora.carry.v1 files no record (N0451) VivariumCarryBridge writes the hand into the frame and
 *              VivariumView::open_gap marks the gap on the edge of the bridge.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_hand_mode.hpp>
#include <ase/adp/godot/godot_hud_ui_icon_class.hpp>
#include <ase/adp/godot/godot_hud_ui_item_sprite.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ase::adp::godot {

/** Where the thing in the hand was drawn (the mockup's POS entry), for the drag layer to lift it from, and the
 *  stow button and the verb chips drawn with it, for hit() and the place lines of the view. */
struct HandThingPlace {
    std::string key;       // the thing
    ::godot::Rect2 box;    // hud_hand_box round the hand, in frame units
    double k = 0.0;        // the scale of its drawing
    ::godot::Rect2 stow;   // the stow button beside it (hud_hand_stow_hit_*), in frame units
    std::vector<std::pair<std::string, ::godot::Rect2>> verbs;  // each verb chip (HandMode): the square round it
};

/** The CPU time of the parts of the last draw with a thing in the hand (P0692), each in microseconds of the drawing
 *  thread; all zero after a draw of the empty hand. VivariumHud names them in hud_draw_split under inventory_hand. */
struct HandSlotCost {
    uint64_t look_usec = 0;  // look() and sprite(): the item's tokens and its drawing, read on first use
    uint64_t art_usec = 0;   // the hexagon and the drawing fitted into it
    uint64_t stow_usec = 0;  // the stack count and the stow button
    uint64_t mode_usec = 0;  // HandMode::draw: what the mode adds, the verb chips
    uint64_t text_usec = 0;  // the name, the instruction, the case and the reading
    uint64_t title_usec = 0;    // of text_usec: the item's name
    uint64_t hint_usec = 0;     // of text_usec: the instruction under it
    uint64_t case_usec = 0;     // of text_usec: the case words, from levels[2]
    uint64_t reading_usec = 0;  // of text_usec: the reading on the right, label and value
    uint64_t value_usec = 0;    // of reading_usec: the value alone, the data face's line under the label
    uint64_t shape_usec = 0;    // of every line of the hand: shaping its glyphs anew (UiTextBlock::shape_usec)
    uint64_t warm_usec = 0;     // readying every line of the hand at its raster (UiTextBlock::warm), before the draw
};

/** The hand slot. */
class HandSlot {
public:
    /** Binds measures, colours, words, icons and levels; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the hand for this frame; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const HandSlotFrame& now,
              std::string& failed);

    /**
     * The act at `point` as last drawn: GODOT_HUD_ACT_VERB with the verb, GODOT_HUD_ACT_STOW, GODOT_HUD_ACT_POUR
     * or GODOT_HUD_ACT_ITEM with the thing's key in `key`; empty for the empty hand and outside.
     */
    std::string hit(const ::godot::Vector2& point, std::string& key) const;

    /** The thing in the hand as last drawn, an empty key while the hand is empty. */
    const HandThingPlace& place() const { return place_; }

    /** The CPU time of the parts of the last draw (P0692). */
    const HandSlotCost& cost() const { return cost_; }

private:
    /** The look of the item `id`, read once; nullptr with the reason in `failed`. */
    const HandItemLook* look(const std::string& id, std::string& failed);

    /** The sprite of the drawing `art`, read once; nullptr with the reason in `failed`. */
    const UiItemSprite* sprite(const std::string& art, std::string& failed);

    bool with_thing(UiPen& canvas, const ::godot::Transform2D& frame, const HandSlotFrame& now,
                    const HandThing& thing, std::string& failed);
    bool without_thing(UiPen& canvas, const ::godot::Transform2D& frame, const HandSlotFrame& now,
                       std::string& failed);

    /** Draws `runs` into `block` at (x, y), adding its shaping to cost_.shape_usec; false with the reason in `failed`. */
    bool say(UiPen& canvas, const ::godot::Transform2D& frame, UiTextBlock& block, const std::vector<HudRun>& runs,
             double x, double y, double size, double anchor, const ::godot::Color& color, std::string& failed);

    const UiTheme* theme_ = nullptr;
    double hand_x_ = 0.0, hand_y_ = 0.0, radius_ = 0.0;                              // hud_hand_x, _y, _radius
    double fill_alpha_ = 0.0, empty_fill_alpha_ = 0.0;                               // hud_hand_*fill_alpha
    double art_ = 0.0, box_ = 0.0, hit_radius_ = 0.0;                                // hud_hand_art, _box, _hit_radius
    double line_ = 0.0, hot_line_ = 0.0, empty_line_ = 0.0, dash_ = 0.0, dash_gap_ = 0.0;
    double empty_glyph_size_ = 0.0, empty_label_dy_ = 0.0, empty_label_size_ = 0.0;  // hud_hand_empty_*
    double hint_y_ = 0.0, hint_size_ = 0.0;                                          // hud_hand_hint_*
    double count_dx_ = 0.0, count_size_ = 0.0;                                       // hud_hand_count_*
    double divider_y_ = 0.0, left_from_ = 0.0, left_to_ = 0.0, right_from_ = 0.0, right_to_ = 0.0;
    double divider_line_ = 0.0;
    double title_x_ = 0.0, title_y_ = 0.0, title_size_ = 0.0;                        // hud_hand_title_*
    double hint_line_y_ = 0.0, hint_line_size_ = 0.0, case_y_ = 0.0, case_size_ = 0.0;
    double reading_x_ = 0.0, label_y_ = 0.0, label_size_ = 0.0, value_y_ = 0.0, value_size_ = 0.0;
    double stow_x_ = 0.0, stow_y_ = 0.0, stow_size_ = 0.0, stow_label_y_ = 0.0, stow_label_size_ = 0.0;
    double stow_hit_x_ = 0.0, stow_hit_y_ = 0.0, stow_hit_w_ = 0.0, stow_hit_h_ = 0.0;
    uint32_t level_words_ = 0, level_detail_ = 0;                                    // hud.perception.levels
    ::godot::Color soft_, faint_, empty_, hot_, refuse_;
    std::vector<HudRun> hand_free_, hint_rest_, stow_, stack_count_;
    UiIcon hand_icon_;         // hand_glyph
    UiIcon stow_icon_;         // stow_glyph
    // One block per line, as UiTextBlock is one line: a block set to another line's runs shapes anew (P0692).
    UiTextBlock title_text_;    // class tb: the name
    UiTextBlock hint_text_;     // class t: the instruction
    UiTextBlock label_text_;    // class t: the reading's label
    UiTextBlock stow_text_;     // class t: the word under the stow button
    UiTextBlock free_text_;     // class t: texts.hud.hand_free under the empty hand
    UiTextBlock rest_text_;     // class t: texts.hud.hint_rest in the mode rest
    UiTextBlock count_text_;    // class d: the stack count
    UiTextBlock case_text_;     // class d: the case words
    UiTextBlock value_text_;    // class d: the reading's value
    HandMode mode_;            // what the mode adds
    std::vector<std::pair<std::string, HandItemLook>> looks_;    // one per item, read on first use
    std::vector<std::pair<std::string, UiItemSprite>> sprites_;  // one per drawing, read on first use
    HandThingPlace place_;                                       // the thing and its stow button as last drawn
    bool pours_ = false;                                         // a tap on the hand pours
    HandSlotCost cost_;                                          // the parts of the last draw (P0692)
};

}  // namespace ase::adp::godot
