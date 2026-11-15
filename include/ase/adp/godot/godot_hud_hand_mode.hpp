#pragma once

/**
 * ASE GODOT ADAPTER - HUD HAND MODE
 *
 * @file        godot_hud_hand_mode.hpp
 * @brief       What the hand says and draws in each mode: the pour arcs, the rain, the verbs, the readings
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block HandSlot, the mode part of the mockup's hand().
 *              HandSlot draws the hand, the thing in it and the stow button; for a thing in the hand it asks
 *              this unit for the lines beside it (HandLines: the instruction, the reading's label and value,
 *              the case) and lets it draw what the mode adds round the hand.
 *
 *              WATER. Instruction texts.hud.water_empty with an empty can, water_release while held, else
 *              water_hold. Reading row_soil: from hud.perception.levels[1] water_after with the soil water now
 *              and after the pour as percentages, below it words.dry when the share after lies under
 *              hud_focus_soil_dry_below, else words.moist. hud_hand_pour_arcs arcs of hud_hand_pour_radius, arc i
 *              from i * hud_hand_pour_arc_step + hud_hand_pour_arc_from to + hud_hand_pour_arc_to, width
 *              hud_hand_pour_arc_width: the tone for each litre poured, DOMAIN_WATER_HELD for each litre still in
 *              the can, else OVERLAY_INK_FAINT. While pouring pour_mm stands hud_hand_pour_text_dy over the hand
 *              (class d); from levels[1] water_fill hud_hand_pour_fill_dy under it. With rain coming the glyph
 *              rain_glyph in DOMAIN_SKY at hud_hand_rain_glyph_* and from levels[2] rain_in (rain_in_one for one
 *              phase) with phases and millimetres, below it rain_coming, at hud_hand_rain_text_*.
 *
 *              PRUNE prune_ask, reading prune_left with the shoots left. SOW sow_none without seeds, sow_ask once
 *              the birch is dead, else sow_alive; reading sow_left with the seeds. SAP sap_no_vessel without the
 *              trough, sap_no_spout without a spout, else sap_ask; reading sap_left with the spouts. PLUG plug_ask,
 *              reading plug_vessel with hand_litres. Every other mode: the hold word of the thing (hud_item_<id>_
 *              hold) or use_hand for a tool at work; reading weight with hand_weight.
 *
 *              TERRAIN. The tool's verbs stand on an arc hud_hand_verb_distance above the hand: one at 0 degrees,
 *              two at -/+ hud_hand_verb_two_angle, three at -hud_hand_verb_three_angle, 0, + the same; each a glyph
 *              chip of hud_hand_verb_radius with verb_glyph.<v> in the verb's tone (hud_verb_<v>_tone), lit for
 *              the chosen verb, and over it verbs.<v> hud_hand_verb_label_dy above, in the tone when chosen, else
 *              OVERLAY_INK_SOFT, when the tool has more than one verb or the perception stands at levels[0]. The
 *              instruction is verb_ask.<op> of the verb's op (hud_verb_<v>_op), for the op bed verb_ask.bed_start
 *              or bed_end; the reading label verb_reading.level with the area, .bed, .measure, else .radius with
 *              the brush; with the brush on the ground and from levels[1] its value .height, from levels[2] with
 *              .slope. The case is verb_case.<v>. While the brush lies in the root zone of the patch, and the op
 *              is neither measure nor level, texts.hud.roots stands in DOMAIN_ALERT at hud_hand_root_text_*.
 *
 *              THE PICTURE IS WHOLE. Every value is drawn from the frame and never asked whether a port
 *              delivered it. While a port is silent the bridge writes the mockup's value into the frame
 *              (VivariumHandBridge, VivariumCarryBridge) and VivariumView::open_gap marks the gap on its edge:
 *              the soil water, the pour and the preview after it N0454 (flora.patch.v1), the rain N0449
 *              (flora.weather.v1), the shoots to cut N0462, whether the birch is dead N0461, the root zone N0463
 *              (flora.root.v1), the sap in the trough N0457, the height and the slope under the brush N0464.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100367 - water a patch
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0454 (flora.patch.v1), N0449 (flora.weather.v1), N0462, N0461, N0463 (flora.root.v1), N0457,
 *              N0464 (height and slope under the brush), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_hex_chip.hpp>
#include <ase/adp/godot/godot_hud_ui_icon.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace ase::adp::godot {

/** The thing in the hand as flora.carry.v1 reports it, or the bridge while the port is silent. */
struct HandThing {
    std::string key;                 // o.key: the thing it is
    std::string id;                  // o.id: the item, which names its words and tokens
    std::string art;                 // the item_art.json name of its drawing
    bool stack = false;              // D.stack: the item stacks
    uint32_t count = 0;              // o.n: how many of it lie in the hand
    double kg = 0.0;                 // wt(it): what it weighs with its content
    double fill_l = 0.0;             // it.fill: the litres a vessel holds
    double cap_l = 0.0;              // the litres a vessel takes
    std::vector<std::string> verbs;  // D.verbs: what a ground tool does, each the key of verbs.<v>
    double level = 0.0;              // the held share 0..1 as D.art(o) draws it (VivariumHudThing::level)
};

/** The hand slot this frame. */
struct HandSlotFrame {
    uint32_t level = 0;                // the depth of perception
    std::string mode;                  // the mode the view is in (hud.modes, GODOT_HUD_MODE_*)
    const HandThing* thing = nullptr;  // the thing in the hand, nullptr while the hand is free
    bool dragged = false;              // the thing in the hand is lifted by a drag: the hand draws empty
    bool hot = false;                  // a drag stands over the hand
    bool hot_bad = false;              // and the hand refuses it
    bool toast_posted = false;         // a toast was posted since the view started
    // water
    double soil_rel = 0.0;             // S.soil: the soil water now, 0..1
    double after_rel = 0.0;            // the soil water after the pour, 0..1
    double pour_mm = 0.0;              // S.pour: the millimetres poured so far
    double poured_l = 0.0;             // the litres poured so far
    bool holding = false;              // S.holding: the finger holds the can over the patch
    bool rain_coming = false;          // rainIn(): rain falls within the forecast (the port's or the bridge's, N0449)
    uint32_t rain_phases = 0;          // ri.n: the phases until it falls
    double rain_mm = 0.0;              // ri.mm: the millimetres it brings
    // prune
    uint32_t prune_left = 0;           // the marked shoots not cut yet
    // sow
    uint32_t seeds = 0;                // seedsLeft(): the seeds in the pouch
    bool dead = false;                 // S.dead
    // terrain
    std::string verb;                  // S.terr: the chosen verb
    bool bed_started = false;          // S.bedA: the start of a brook bed is set
    std::string area;                  // {place}: the area a level act covers
    double brush_m = 0.0;              // the brush's radius in metres
    bool brush_on = false;             // S.brush: the brush lies on the ground
    double height_m = 0.0;             // hAt(brush)
    double slope_deg = 0.0;            // slopeAt(brush)
    bool in_roots = false;             // the brush overlaps the root zone
    // sap and plug
    bool has_vessel = false;           // have('cocho')
    uint32_t spouts = 0;               // findIt('bica').n
    double tap_l = 0.0;                // S.tap.l
    double elapsed = 0.0;              // the seconds since the mode opened
    bool still = false;                // reduced motion: every animation at its end
};

/** What a thing in the hand is drawn with: read once per item. */
struct HandItemLook {
    ::godot::Color tone;               // DOMAIN_<hud_item_<id>_tone>
    std::vector<HudRun> name;          // items.<id>.name
    std::vector<HudRun> hold;          // texts.hud.<hud_item_<id>_hold>, empty for a tool at work
    std::vector<HudRun> case_words;    // items.<id>.case
};

/** The lines beside the hand, as the mode gives them. */
struct HandLines {
    std::vector<HudRun> hint;          // ins: the instruction
    std::vector<HudRun> label;         // rl: the reading's label, empty for none
    std::vector<HudRun> value;         // rv: the reading's value, empty for none
    std::vector<HudRun> case_words;    // uc: the case under the instruction
};

/** The mode part of the hand slot. */
class HandMode {
public:
    static constexpr const char* GAP_MEASURE = "N0464";  // height and slope under the brush on a port

    /** Binds measures, colours, words and levels; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws what the mode adds round the hand and fills `lines`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const HandSlotFrame& now,
              const HandThing& thing, const HandItemLook& look, HandLines& lines, std::string& failed);

    /** The verb chip at `point` as last drawn: GODOT_HUD_ACT_VERB with the verb in `key`, else empty. */
    std::string hit(const ::godot::Vector2& point, std::string& key) const;

    /** The verb chips as last drawn, the same hit() asks: the verb and its hexagon, in the frame's units. */
    const std::vector<std::pair<std::string, UiHexChipPlace>>& verb_places() const { return verb_places_; }

    /**
     * Readies the lines an act brings (the root zone, the rain, the pour, the fill) at their raster (P0692), from the
     * glyphs `budget` still allows this frame (UiTextBlock::warm).
     */
    void warm(const UiPen& canvas, const ::godot::Transform2D& frame, int64_t& budget);

private:
    struct VerbChip {
        std::string verb;
        UiHexChip chip;
        ::godot::Color tone;
        std::string op;
        std::vector<HudRun> name;
        std::vector<HudRun> case_words;
        UiTextBlock name_text;  // class t: the verb's name under its chip, its own line (P0692)
    };

    bool water(UiPen& canvas, const ::godot::Transform2D& frame, const HandSlotFrame& now,
               const HandThing& thing, const ::godot::Color& tone, HandLines& lines, std::string& failed);
    bool terrain(UiPen& canvas, const ::godot::Transform2D& frame, const HandSlotFrame& now,
                 const HandThing& thing, HandLines& lines, std::string& failed);

    /** The chip of `verb`, bound on first use; nullptr with the reason in `failed`. */
    VerbChip* chip(const std::string& verb, std::string& failed);

    /** `word` filled with `values` into `out`; false with the reason in `failed`. */
    bool fill(const std::vector<HudRun>& word, const std::vector<HudPortValue>& values,
              std::vector<HudRun>& out, std::string& failed) const;

    /** The word texts.hud.<key>; false with the missing key in `failed`. */
    bool word(const std::string& key, std::vector<HudRun>& out, std::string& failed) const;

    /** Draws `runs` into `block` at (x, y); false with the reason in `failed`. */
    static bool say(UiPen& canvas, const ::godot::Transform2D& frame, UiTextBlock& block,
                    const std::vector<HudRun>& runs, double x, double y, double size, double anchor,
                    const ::godot::Color& color, std::string& failed);

    /** A count as the mockup prints it. */
    static std::string count(uint32_t value);

    const UiTheme* theme_ = nullptr;
    double hand_x_ = 0.0, hand_y_ = 0.0;                                             // hud_hand_x, _y
    double pour_radius_ = 0.0, pour_step_ = 0.0, pour_from_ = 0.0, pour_to_ = 0.0;  // hud_hand_pour_*
    double pour_width_ = 0.0, pour_arcs_ = 0.0;
    double pour_text_dy_ = 0.0, pour_text_size_ = 0.0, pour_fill_dy_ = 0.0, pour_fill_size_ = 0.0;
    double rain_glyph_x_ = 0.0, rain_glyph_y_ = 0.0, rain_glyph_size_ = 0.0;        // hud_hand_rain_glyph_*
    double rain_text_x_ = 0.0, rain_text_y_ = 0.0, rain_text_size_ = 0.0;           // hud_hand_rain_text_*
    double root_x_ = 0.0, root_y_ = 0.0, root_size_ = 0.0;                          // hud_hand_root_text_*
    double verb_distance_ = 0.0, verb_radius_ = 0.0, verb_two_ = 0.0, verb_three_ = 0.0;  // hud_hand_verb_*
    double verb_label_dy_ = 0.0, verb_label_size_ = 0.0;
    double dry_below_ = 0.0;                                                         // hud_focus_soil_dry_below
    double decimals_ = 0.0, fine_below_ = 0.0, fine_decimals_ = 0.0;                 // hud_load_*
    uint32_t level_words_ = 0, level_values_ = 0, level_detail_ = 0;                 // hud.perception.levels
    ::godot::Color held_, soft_, faint_, sky_, alert_;
    std::vector<HudRun> water_empty_, water_release_, water_hold_, water_after_, water_fill_, row_soil_;
    std::vector<HudRun> dry_, moist_, rain_in_, rain_in_one_, rain_coming_, pour_mm_;
    std::vector<HudRun> prune_ask_, prune_left_, sow_none_, sow_ask_, sow_alive_, sow_left_;
    std::vector<HudRun> sap_no_vessel_, sap_no_spout_, sap_ask_, sap_left_, plug_ask_, plug_vessel_;
    std::vector<HudRun> use_hand_, weight_, number_, kg_, litres_, roots_;
    std::vector<HudRun> bed_start_, bed_end_, read_level_, read_bed_, read_measure_, read_radius_;
    std::vector<HudRun> read_height_, read_slope_, read_plus_;
    // One block per line, as UiTextBlock is one line: a block set to another line's runs shapes anew (P0692).
    UiTextBlock label_text_;   // class t: the root zone under the brush
    UiTextBlock rain_text_;    // class t: the rain the forecast brings
    UiTextBlock data_text_;    // class d: the millimetres of the pour
    UiTextBlock fill_text_;    // class d: the can's fill and capacity
    UiRing ring_;              // the pour arcs
    UiIcon rain_icon_;         // rain_glyph
    std::vector<VerbChip> chips_;                                   // one per verb, bound on first use
    std::vector<std::pair<std::string, UiHexChipPlace>> verb_places_;  // the verb chips as last drawn
};

}  // namespace ase::adp::godot
