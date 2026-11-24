#pragma once

/**
 * ASE GODOT ADAPTER - HUD SIGN READING
 *
 * @file        godot_hud_sign_reading.hpp
 * @brief       The reading of the open sign: leader, title, sentence, bar, numbers, case and the offer of its tool
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block SignMarker, its open part (the mockup's
 *              signOpen()). SignMarker places the sign and hands its centre over.
 *
 *              THE COLUMN. The reading opens to the right where the sign stands left of hud_sign_split_x, to the
 *              left otherwise (dir = +1 or -1). The elbow stands at (x + dir * hud_sign_elbow_dx, y +
 *              hud_sign_elbow_dy); the column at bx, the elbow moved hud_sign_column_gap on and held no further
 *              than hud_sign_column_right_max to the right and no nearer than hud_sign_column_left_min to the
 *              left. A leader in the sign's tone at hud_line runs from (x + dir * hud_sign_anchor_gap, y -
 *              hud_sign_anchor_gap) over the elbow hud_sign_run along. Every line is anchored at its start to the
 *              right and at its end to the left:
 *                texts.hud.signs.<id>.title (class tb, in the tone) at the elbow's height + hud_sign_title_dy;
 *                texts.hud.signs.<id>.line (class t, OVERLAY_INK) hud_sign_sentence_dy under the elbow, the next
 *                line hud_sign_line_step under it; a sign with an alert mark shows .title_full and .line_full
 *                while that mark holds;
 *                from levels[1] on, for a sign whose hud_sign_<id>_bar names a port value, a bar
 *                hud_sign_bar_w by hud_sign_bar_h at hud_sign_bar_dy, its track OVERLAY_INK_FAINT and its share in
 *                the tone, then hud_sign_bar_step on;
 *                from levels[2] on .reading (class d, OVERLAY_INK, hud_sign_numbers_*) and .case (class d,
 *                OVERLAY_INK_FAINT, hud_sign_case_*);
 *                the offer of the sign's tool hud_sign_offer_dx along and hud_sign_offer_dy under the last line:
 *                a thing chip of hud_sign_offer_thing_radius with the drawing of the tool's item, or a glyph chip
 *                of hud_sign_offer_glyph_radius with texts.hud.fan_glyph.<tool>, in hud_fan_<tool>_tone; beside it
 *                texts.hud.take with the item's name, texts.hud.missing where the item is not in possession, or
 *                the word hud_fan_<tool>_offer of a tool without an item, in the tool's tone or
 *                OVERLAY_INK_FAINT where it is missing; from levels[2] on, for an item in possession, where it
 *                lies (texts.hud.where.*).
 *              Every line fades in with hud_anim_fade after its hud_sign_*_delay; the leader draws with
 *              hud_anim_draw.
 *
 *              FULL FROM THE FRAME. The reading draws every value its frame carries and never asks whether a
 *              port delivered it: SignValue holds the text and the share and nothing else, and the place of the
 *              offered item is the word SignPlace names for it. Until the ports report them, the bridge writes the
 *              mockup's values (VivariumView::bridge_signs, VivariumCarryBridge::fill) and marks each gap in its
 *              own source: N0461 the sign cases, N0451 (flora.carry.v1) the place of a thing. An item no place is
 *              named for shows no place line, as the mockup's where() of no thing is empty.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @category    ecs/module
 * @usecase     usecase:100370 - wilt from drought
 * @actor       Hänge-Birke (Betula pendula)
 * @register    N0461 (the state fields of each sign case), N0451 (the place of a thing), N0457 (birch sap),
 *              N0460 (season and light), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_context_resolver.hpp>
#include <ase/adp/godot/godot_hud_ui_hex_chip.hpp>
#include <ase/adp/godot/godot_hud_ui_item_sprite.hpp>
#include <ase/adp/godot/godot_hud_ui_leader_line.hpp>
#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One port value a sign's reading or bar names. */
struct SignValue {
    std::string name;            // the placeholder of the reading or the name hud_sign_<id>_bar gives
    std::string text;            // the value as it is shown in the reading
    double share = 0.0;          // 0..1: the share a bar of this value fills
};

/** Where one item in possession lies: the mockup's where(findIt(id)). */
struct SignPlace {
    std::string item;            // the key of hud.items
    const char* where = nullptr; // GODOT_HUD_WHERE_*: the word of its first carried piece's place
};

/** What the open reading shows this frame. */
struct SignReadingFrame {
    HudSignShown shown;                       // the open sign as the context decided it
    ::godot::Vector2 centre;                  // where SignMarker placed it, in mockup units
    uint32_t level = 0;                       // the perception level (hud.perception.levels)
    const std::vector<SignValue>* values = nullptr;  // the values the readings and bars name
    const std::vector<SignPlace>* places = nullptr;  // where each item in possession lies
    double opened = 0.0;                      // seconds since the sign opened
    bool still = false;                       // every motion held
};

/** The reading of the open sign. */
class SignReading {
public:
    /** Binds measures, colours, words, tools and levels; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the reading of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const SignReadingFrame& now,
              std::string& failed);

    /** Whether `point` (in mockup units) hits the offer chip drawn last; its tool in `tool`. */
    bool hit(const ::godot::Vector2& point, uint32_t& tool) const;

private:
    /** The words, tone and bar of one sign. */
    struct Sign {
        ::godot::Color tone;                // hud_sign_<id>_tone
        std::string bar;                    // hud_sign_<id>_bar, empty where it is none
        std::vector<HudRun> title;          // texts.hud.signs.<id>.title
        std::vector<HudRun> line;           // texts.hud.signs.<id>.line
        std::vector<HudRun> title_full;     // .title_full of a sign with an alert mark
        std::vector<HudRun> line_full;      // .line_full of a sign with an alert mark
        std::vector<HudRun> reading;        // texts.hud.signs.<id>.reading
        std::vector<HudRun> case_words;     // texts.hud.signs.<id>.case
    };

    /** One tool a sign can offer. */
    struct Offer {
        ::godot::Color tone;                // hud_fan_<tool>_tone
        bool thing = false;                 // the tool serves through an item
        std::string icon;                   // texts.hud.fan_glyph.<tool> of a tool without an item
        std::string has;                    // hud.tools.<tool>.has: the item of a tool with one
        std::string item;                   // texts.hud.items.<has>.name of a tool with one
        std::vector<HudRun> word;           // texts.hud.<hud_fan_<tool>_offer> of a tool without an item
        UiItemSprite drawing;               // the item's drawing of a tool with an item
    };

    /** The column of one reading, as draw() computes it. */
    struct Column {
        double bx = 0.0;                    // the x every line is anchored at
        double dir = 0.0;                   // +1 to the right, -1 to the left
        double anchor = 0.0;                // GODOT_TEXT_ANCHOR_START or _END
    };

    bool bind_sign(const UiTheme& theme, const HudSignSpec& spec, Sign& out, std::string& failed) const;
    bool bind_offer(const UiTheme& theme, const HudToolSpec& spec, Offer& out, std::string& failed) const;

    /** The bar of the value the sign names, at the line `yy`. */
    bool draw_bar(UiPen& canvas, const ::godot::Transform2D& frame, const SignReadingFrame& now,
                  const Sign& sign, const Column& column, double yy, std::string& failed) const;

    /** The reading line at `yy`, every placeholder filled from the frame's values. */
    bool draw_numbers(UiPen& canvas, const ::godot::Transform2D& frame, const SignReadingFrame& now,
                      const Sign& sign, const Column& column, double yy, std::string& failed);

    /** The offer chip, its word and the place of its item, the last line at `yy`. */
    bool draw_offer(UiPen& canvas, const ::godot::Transform2D& frame, const SignReadingFrame& now,
                    const Column& column, double yy, std::string& failed);

    /** One line of `block` at (bx, y) in `color` after `delay`. */
    void draw_line(UiPen& canvas, const ::godot::Transform2D& frame, const SignReadingFrame& now,
                   UiTextBlock& block, const Column& column, double y, double size, const ::godot::Color& color,
                   double delay) const;

    double split_x_ = 0.0;              // hud_sign_split_x
    double anchor_gap_ = 0.0;           // hud_sign_anchor_gap
    double elbow_dx_ = 0.0;             // hud_sign_elbow_dx
    double elbow_dy_ = 0.0;             // hud_sign_elbow_dy
    double column_gap_ = 0.0;           // hud_sign_column_gap
    double column_right_max_ = 0.0;     // hud_sign_column_right_max
    double column_left_min_ = 0.0;      // hud_sign_column_left_min
    double run_ = 0.0;                  // hud_sign_run
    double title_dy_ = 0.0;             // hud_sign_title_dy
    double title_size_ = 0.0;           // hud_sign_title_size
    double title_delay_ = 0.0;          // hud_sign_title_delay
    double sentence_dy_ = 0.0;          // hud_sign_sentence_dy
    double sentence_size_ = 0.0;        // hud_sign_sentence_size
    double sentence_delay_ = 0.0;       // hud_sign_sentence_delay
    double line_step_ = 0.0;            // hud_sign_line_step
    double bar_w_ = 0.0;                // hud_sign_bar_w
    double bar_h_ = 0.0;                // hud_sign_bar_h
    double bar_dy_ = 0.0;               // hud_sign_bar_dy
    double bar_delay_ = 0.0;            // hud_sign_bar_delay
    double bar_step_ = 0.0;             // hud_sign_bar_step
    double numbers_size_ = 0.0;         // hud_sign_numbers_size
    double numbers_delay_ = 0.0;        // hud_sign_numbers_delay
    double numbers_step_ = 0.0;         // hud_sign_numbers_step
    double case_size_ = 0.0;            // hud_sign_case_size
    double case_delay_ = 0.0;           // hud_sign_case_delay
    double case_step_ = 0.0;            // hud_sign_case_step
    double offer_dx_ = 0.0;             // hud_sign_offer_dx
    double offer_dy_ = 0.0;             // hud_sign_offer_dy
    double offer_thing_radius_ = 0.0;   // hud_sign_offer_thing_radius
    double offer_glyph_radius_ = 0.0;   // hud_sign_offer_glyph_radius
    double offer_delay_ = 0.0;          // hud_sign_offer_delay
    double offer_label_dx_ = 0.0;       // hud_sign_offer_label_dx
    double offer_label_dy_ = 0.0;       // hud_sign_offer_label_dy
    double offer_label_size_ = 0.0;     // hud_sign_offer_label_size
    double offer_label_delay_ = 0.0;    // hud_sign_offer_label_delay
    double offer_where_dy_ = 0.0;       // hud_sign_offer_where_dy
    double offer_where_size_ = 0.0;     // hud_sign_offer_where_size
    double offer_where_delay_ = 0.0;    // hud_sign_offer_where_delay
    double line_ = 0.0;                 // hud_line: the leader
    uint32_t level_bars_ = 0;           // hud.perception.levels[1]
    uint32_t level_numbers_ = 0;        // hud.perception.levels[2]
    ::godot::Color ink_;                // OVERLAY_INK
    ::godot::Color faint_;              // OVERLAY_INK_FAINT
    std::vector<Sign> signs_;           // one per entry of hud.signs, in its order
    std::vector<Offer> offers_;         // one per entry of hud.tools, in its order
    std::vector<HudRun> take_words_;    // texts.hud.take
    std::vector<HudRun> missing_words_; // texts.hud.missing
    const UiTheme* theme_ = nullptr;    // the theme the place words are read from
    UiMotion fade_;                     // hud_anim_fade
    UiLeaderLine leader_;               // the leader
    UiHexChip chip_;                    // the offer chip
    UiTextBlock title_;                 // class tb
    UiTextBlock sentence_;              // class t
    UiTextBlock reading_;               // class d
    UiTextBlock case_;                  // class d
    UiTextBlock offer_label_;           // class t
    UiTextBlock where_;                 // class d
    UiHexChipPlace offer_place_;        // the offer chip drawn last, radius 0 where none
    uint32_t offer_tool_ = UINT32_MAX;  // the tool of that chip
};

}  // namespace ase::adp::godot
