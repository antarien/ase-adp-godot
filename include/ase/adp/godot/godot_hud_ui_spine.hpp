#pragma once

/**
 * ASE GODOT ADAPTER - HUD UI SPINE
 *
 * @file        godot_hud_ui_spine.hpp
 * @brief       The spine right of the focus ring: a column of rows, each a dot, a label, a value and a bar
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.3, primitive UiSpine. The mockup's focus() lays the
 *              rows of the focused plant (stage, soil water, cover, and the rows a state adds: loose soil, sap,
 *              condition) down the column SX = TX + hud_focus_column_dx. Row i stands at the baseline
 *              y = hud_focus_row_y + i * hud_focus_row_step and draws:
 *                the dot     a disc of hud_focus_row_dot_radius at (SX, y + hud_focus_row_dot_dy) in the row's
 *                            tone, fading in after hud_focus_row_dot_delay + i * hud_focus_row_stagger
 *                the tick    a leader line from SX + hud_focus_row_tick_from to SX + hud_focus_row_tick_to at
 *                            the dot's height, in OVERLAY_INK_FAINT
 *                the label   class t at SX + hud_focus_title_dx, hud_focus_row_label_size, OVERLAY_INK_SOFT,
 *                            fading in after hud_focus_row_label_delay + i * stagger
 *                the value   end-anchored at hud_focus_row_value_x: class d at hud_focus_row_value_size when it
 *                            carries a digit, class t at hud_focus_row_value_strong_size when it is a word; in
 *                            the row's tone for an alert row, OVERLAY_INK otherwise; fading in after
 *                            hud_focus_row_value_delay + i * stagger
 *                the bar     with a share: a track of hud_focus_bar_h in OVERLAY_INK_FAINT over the label's
 *                            start to the value's end at y + hud_focus_bar_dy, and the share of it filled at
 *                            hud_focus_bar_strong_h in the row's tone
 *                the pips    on a row that carries them: hud_focus_pip_count marks of hud_focus_pip_w by
 *                            hud_focus_pip_h, hud_focus_pip_step apart at y + hud_focus_pip_dy, the first
 *                            `pips_lit` of them in the row's tone, the rest in OVERLAY_INK_FAINT
 *                the source  in the debug view: class d at hud_focus_debug_size in DEBUG_INK, end-anchored
 *                            under the value at y + hud_focus_debug_dy - the port the row reads
 *
 *              WHAT A ROW SAYS IS THE CALLER'S. Which rows stand, the words of the perception level (SECO or
 *              a percentage), whether a bar shows and which port a row names come from the group that knows
 *              the plant (FocusCluster); this unit lays out and draws what it is handed.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-25
 * @version     00.00.39.00039 [core]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_leader_line.hpp>
#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One row of the spine as the group hands it. */
struct UiSpineRow {
    std::string label;          // the row's name, e.g. the word of texts.hud.focus_stage
    std::string value;          // the row's value: a word, or a number with its unit
    ::godot::Color tone;        // the domain tone of the row (DOMAIN_*)
    bool alert = false;         // the value is drawn in the tone instead of OVERLAY_INK
    bool bar = false;           // a bar shows under the row
    double share = 0.0;         // the filled share of the bar, 0 to 1
    bool pips = false;          // the row carries the stage pips
    int32_t pips_lit = 0;       // the pips drawn in the tone, counted from the first
    std::string source;         // the port the row reads, shown in the debug view
};

/** The text blocks of one drawn row, kept so a row is shaped again only when its words change. */
struct UiSpineLine {
    UiTextBlock label;   // class t
    UiTextBlock number;  // class d: a value with a digit
    UiTextBlock word;    // class t: a value without one
    UiTextBlock source;  // class d: the debug source
    bool digits = false;  // the value carries a digit, so `number` draws it
};

/** The spine of the focus: its rows laid down one column. */
class UiSpine {
public:
    /** Binds every token and colour of the rows; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The rows of this frame; false when a row's text cannot be set, with the reason in `failed`. */
    bool set(const std::vector<UiSpineRow>& rows, std::string& failed);

    /**
     * Draws the rows down the column at `column_x` (SX) under `frame`, `elapsed` seconds after the focus
     * opened; `still` puts every animation at its end, `debug` adds each row's source. While `rail` (the body rail
     * draws in the same view) the values and bars end at the rail's content (spine_value_end, B0937).
     */
    void draw(UiPen& canvas, const ::godot::Transform2D& frame, double column_x, double elapsed,
              bool still, bool debug, bool rail);

    /**
     * The widest row of this frame measured from the column to the rail's content while the rail draws (B1001): the
     * text inset, the label, the inset again and the value, plus the room the values leave before the rail's content.
     */
    double need(const UiPen& canvas, const ::godot::Transform2D& frame);

    /** hud_survival_rail_track_left: where the body rail's content begins, the bound the card yields to. */
    double rail_left() const { return rail_left_; }

private:
    const UiTheme* theme_ = nullptr;   // the theme the text blocks of a new row bind against
    UiLeaderLine line_;                // the tick
    UiMotion fade_;                    // hud_anim_fade: the dot's fade-in
    std::vector<UiSpineRow> rows_;     // the rows as set
    std::vector<UiSpineLine> lines_;   // their text blocks
    ::godot::Color ink_;               // OVERLAY_INK
    ::godot::Color soft_;              // OVERLAY_INK_SOFT
    ::godot::Color faint_;             // OVERLAY_INK_FAINT
    ::godot::Color debug_;             // DEBUG_INK
    double row_y_ = 0.0;               // hud_focus_row_y
    double row_step_ = 0.0;            // hud_focus_row_step
    double stagger_ = 0.0;             // hud_focus_row_stagger
    double dot_radius_ = 0.0;          // hud_focus_row_dot_radius
    double dot_dy_ = 0.0;              // hud_focus_row_dot_dy
    double dot_delay_ = 0.0;           // hud_focus_row_dot_delay
    double tick_from_ = 0.0;           // hud_focus_row_tick_from
    double tick_to_ = 0.0;             // hud_focus_row_tick_to
    double text_dx_ = 0.0;             // hud_focus_title_dx
    double label_size_ = 0.0;          // hud_focus_row_label_size
    double label_delay_ = 0.0;         // hud_focus_row_label_delay
    double value_x_ = 0.0;             // hud_focus_row_value_x
    double rail_left_ = 0.0;           // hud_survival_rail_track_left: where the body rail's content begins
    double value_size_ = 0.0;          // hud_focus_row_value_size
    double value_strong_size_ = 0.0;   // hud_focus_row_value_strong_size
    double value_delay_ = 0.0;         // hud_focus_row_value_delay
    double bar_dy_ = 0.0;              // hud_focus_bar_dy
    double bar_h_ = 0.0;               // hud_focus_bar_h
    double bar_strong_h_ = 0.0;        // hud_focus_bar_strong_h
    double pip_count_ = 0.0;           // hud_focus_pip_count
    double pip_step_ = 0.0;            // hud_focus_pip_step
    double pip_w_ = 0.0;               // hud_focus_pip_w
    double pip_h_ = 0.0;               // hud_focus_pip_h
    double pip_dy_ = 0.0;              // hud_focus_pip_dy
    double debug_dy_ = 0.0;            // hud_focus_debug_dy
    double debug_size_ = 0.0;          // hud_focus_debug_size
};

}  // namespace ase::adp::godot
