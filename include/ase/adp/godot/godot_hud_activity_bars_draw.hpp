#pragma once

/**
 * ASE GODOT ADAPTER - HUD ACTIVITY BARS BLOCK
 *
 * @file        godot_hud_activity_bars_draw.hpp
 * @brief       ActivityBars: the bars of the time slices of the led figure on the bottom band, right of the hand
 * @description MIG_ASE_SKILL_RSM step B5, usecase:101305 show skill progress. A block of the layer inventory, drawn
 *              after HandSlot, BeltHolster and BagIcon (row 13 of the table "Ebenen"): it shows wherever the inventory
 *              shows and hides by the same rule hud.modes.hides.
 *
 *              WHAT IT SHOWS: one bar per time slice of avatar.skill.bar.v1 of the led figure, the youngest left
 *              (godot_hud_activity_bars.hpp), each as tall as its quantity times hud_bars_unit, filled in the tone of
 *              the domain of its action. A lit action fills its bars in full cover and edges them in the HUD white
 *              with the stroke hud_line, every other bar is filled at hud_bars_dim_alpha; while an action is lit the
 *              sum of the quantities of its bars stands in the data face at hud_bars_text_x, hud_bars_text_y.
 *              THE LIT ACTION: the action of the bar a finger holds (the HUD sets ActivityBarsFrame::held); while no
 *              finger holds one, every action whose tool is in the hand (hud.bars.<code>.items names it).
 *              THE DOMAIN AND THE TOOLS OF AN ACTION are data, hud.bars of the start configuration, keyed by the
 *              action code BDI_CONST_ACTION_* the server sends: tone names a domain tone of hud.tones (DOMAIN_<TONE>
 *              of colors.json), items the items of hud.items whose holding lights it. A bar of a code hud.bars does
 *              not name stops the drawing with its code in `failed`: a colour of its own would be a guess.
 *              THE PLACE: the strip hud_bars_x .. + 20 bars, hud_bars_base - hud_bars_h .. hud_bars_base, measured
 *              against the hits of row 13: right of the hand's hit (x 214) and of the right verb of three (x 227.3),
 *              left of the put-away hit (x 300), above the right holster (y 584) and the put-away hit (y 574), under
 *              the hand's divider (y 552).
 *              IT ASKS NOTHING: hit() names the action of the bar under a touch; the HUD keeps it lit while the finger
 *              stays and sends no request for it.
 *              THE BAND (operator order 2026-11-25, godot_hud_activity_bars.hpp): the block keeps what the bottom band
 *              shows (step() once a frame, toggle() on a tap of the switch, band() for the HUD); on the bars they stand on the whole band under the layout hud_band_bars_*
 *              (x 27 .. 333, foot y 628, at most 60 tall: under the dividers at y 552) and HandSlot, BeltHolster and
 *              BagIcon stand hidden; on the slots they stand in the strip above. THE SWITCH stands in the gap of the
 *              dividers over the hand (hud_band_switch_*) while there is a bar to turn to; it names the content a tap
 *              turns the band to (texts.hud.band.slots, texts.hud.band.bars), switch_hit() answers the tap.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_074
 * @category    ecs/module
 * @usecase     usecase:101305 - show skill progress
 * @actor       Skill Measurement Tracker (SKILMEATRA)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.02.00002 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_activity_bars.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** What the bars show this frame. */
struct ActivityBarsFrame {
    std::vector<ActivityBar> bars;     // avatar.skill.bar.v1 of the led figure as the port laid it, written by the view
    std::string hand;                  // the item in the hand (HandThing::id), set by the HUD; empty while the hand is free
    uint32_t held = UINT32_MAX;        // the action of the bar a finger holds, set by the HUD; GODOT_RSM_ACTION_NONE: none
};

/** One action of hud.bars: the colour of its domain and the items whose holding lights it. */
struct ActivityBarsAction {
    uint32_t code = 0;                 // BDI_CONST_ACTION_*: the key of hud.bars
    ::godot::Color tone;               // DOMAIN_<TONE> of hud.bars.<code>.tone
    std::vector<std::string> items;    // hud.bars.<code>.items, each an item of hud.items
};

/** The bars of the time slices of the led figure. */
class ActivityBars {
public:
    /** Binds the tokens hud_bars_*, the HUD white and hud.bars of `root`; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, const ::godot::Dictionary& root, std::string& failed);

    /** Turns the band for this frame (activity_band_step): the bars of `bars` at the wall seconds `wall`, the
     *  inventory in use where `needs_slots`, and the switch where toggle() was called since the last step. */
    void step(const std::vector<ActivityBar>& bars, double wall, bool needs_slots);

    /** The switch was tapped: the next step() turns the band to the other content and holds it there. */
    void toggle();

    /** What the bottom band shows (GODOT_RSM_BAND_*): the HUD hides the slots while it shows the bars. */
    uint32_t band() const;

    /** Draws the bars of `now` and the band switch under `frame`; false with the code hud.bars does not name in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const ActivityBarsFrame& now, std::string& failed);

    /** The action of the bar a touch at `point` chooses, as last drawn, in `action`; false off the bars. */
    bool hit(const ::godot::Vector2& point, uint32_t& action) const;

    /** Whether a touch at `point` lies on the band switch as last drawn. */
    bool switch_hit(const ::godot::Vector2& point) const;

    /** The strip a touch reaches the bars on (in the mockup's units) in `out`; false while no bar was drawn. */
    bool place(::godot::Rect2& out) const;

    /** The band switch (in the mockup's units) in `out`; false while it was not drawn. */
    bool switch_place(::godot::Rect2& out) const;

    /** The bars as last drawn (in the mockup's units, the youngest first, each with its action and whether it is lit)
     *  in `out`; empty while no bar was drawn. The acceptance's place lines aim a hold at one of them (B6). */
    void bars(std::vector<ActivityBarPlace>& out) const;

private:
    /** Draws the switch: its frame and the word of the content a tap turns the band to. */
    void draw_toggle(UiPen& canvas, const ::godot::Transform2D& frame, uint32_t band);

    ActivityBarsLayout layout_;                 // hud_bars_x, _base, _w, _gap, _unit, _h, _hit_pad: the strip beside the slots
    ActivityBarsLayout band_layout_;            // hud_band_bars_*: the bars on the whole band, the slots hidden
    double dim_ = 0.0;                          // hud_bars_dim_alpha: the cover of a bar whose action is not lit
    double line_ = 0.0;                         // hud_line: the edge of a lit bar
    ::godot::Vector2 text_at_;                  // hud_bars_text_x, hud_bars_text_y: the baseline of the lit sum
    ::godot::Vector2 band_text_at_;             // hud_band_bars_text_x, _text_y: the lit sum over the bars of the band
    double text_size_ = 0.0;                    // hud_bars_text_size
    ::godot::Rect2 switch_;                     // hud_band_switch_x, _y, _w, _h: the switch in the gap of the dividers over the hand
    double switch_text_size_ = 0.0;             // hud_band_switch_text_size
    double switch_text_dy_ = 0.0;               // hud_band_switch_text_dy: the baseline of the word below the switch's middle
    double switch_alpha_ = 0.0;                 // hud_band_switch_alpha: the cover of the switch's ground
    ::godot::Color ground_;                     // OVERLAY_SHADOW at hud_band_switch_alpha: the ground of the switch
    std::vector<UiTextBlock> words_;            // texts.hud.band.slots, texts.hud.band.bars, by GODOT_RSM_BAND_*
    const ActivityBarsLayout* drawn_layout_ = nullptr;  // the layout of the last draw: hit() and place() ask it
    ActivityBandState band_;                    // what the bottom band shows and since when (godot_hud_activity_bars.hpp)
    bool toggled_ = false;                      // toggle() was called since the last step()
    bool switch_drawn_ = false;                 // the switch stood in the last frame: there was a bar to turn to
    ::godot::Color ink_;                        // OVERLAY_INK: the HUD white of the edge and the sum
    std::vector<ActivityBarsAction> actions_;   // hud.bars
    UiTextBlock data_;                          // class d: the sum of the lit bars
    std::vector<ActivityBar> ordered_;          // the bars of the last frame, the youngest first
    std::vector<uint32_t> lit_;                 // the lit actions of the last frame
    std::vector<ActivityBarPlace> places_;      // the bars as last drawn

    bool drawn_ = false;
};

}  // namespace ase::adp::godot
