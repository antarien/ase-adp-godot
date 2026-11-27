#pragma once

/**
 * ASE GODOT ADAPTER - HUD DEBUG RSM TABLE DRAWING
 *
 * @file        godot_hud_debug_rsm_draw.hpp
 * @brief       The RSM table of the debug layer drawn: the prototype's timer grid, a column per learning entry
 * @description MIG_ASE_SKILL_RSM step B4, usecase:101305 show skill progress. DebugLayer draws it in the place of the
 *              four status lines while the view RSM is chosen; what it prints and in which order is
 *              godot_hud_debug_rsm.hpp.
 *
 *              THE TABLE IS THE GRID OF THE PROTOTYPE (rsmprototypvolumebar.PNG, PLAN_ASE_VIVARIUM_RSM_REF "Die
 *              Timer-Tabelle", operator 2026-11-25). A column arises with the learning entry of a skill when the skill
 *              is first used, and the table grows by one column per skill, at most hud_debug_rsm_cols, the newest left
 *              (debug_rsm_shown). A column is GODOT_RSM_GRID_ROWS cells SES, ACD, TUC, DCD, QNT, TUL, each written
 *              "KEY: value" (texts.hud.debug_rsm.cell with the row word texts.hud.debug_rsm.<row>) at hud_debug_rsm_pad
 *              in the data face, class d: HH:MM:SS for SES, TUC and TUL, whole numbers for ACD, DCD and QNT. A cell is
 *              hud_debug_rsm_col_w wide and hud_debug_rsm_cell_h high, its baseline hud_debug_rsm_row_step below the
 *              last, its top hud_debug_rsm_top_dy above its baseline, filled DEBUG_CELL under DEBUG_FIELD grid lines of
 *              hud_debug_rsm_rule_line; the SES cell of an entry in phase GODOT_RSM_RED_PHASE (it learns now) is filled
 *              DEBUG_RUN. No head row, as the prototype has none. The portrait drawer lays the columns in bands of
 *              hud_debug_rsm_band_cols from hud_debug_x, hud_debug_y, each band hud_debug_rsm_rule_gap under the last.
 *
 *              THE TABLE STANDS ALWAYS (operator 2026-11-25): a frame whose port names no entry shows the set shown
 *              last with its last values (debug_rsm_shown); before the first learning order one column of zeros.
 *
 *              The rows QUE, LCK, EXP, PRG, VEL, MSR, VOL, the action and the phase word stay bound and computed
 *              (numbers(), words()); the grid of the prototype draws the six rows alone (operator 2026-11-25).
 *
 *              THE RINGS of the shares are not drawn in the tab: it carries the timer table of the prototype alone
 *              (operator 2026-11-25). The two wheels of the prototype, DCD and ACD, stand in the game HUD as SkillRings
 *              (godot_hud_skill_rings_draw.hpp, MIG_ASE_SKILL_RSM B5). ring() with the tokens hud_debug_rsm_ring_* is
 *              the ring of the shares QNT and TUC, kept while no migration serves them (R015).
 *
 *              The measures are tokens of SHA (hud_debug_rsm_*), the tones DEBUG_* of colors.ts, the words of
 *              sha-web-translations/hud.
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

#include <ase/adp/godot/godot_hud_debug_rsm.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_text_place.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** The RSM table of the debug layer. */
class DebugRsmTable {
public:
    /** Binds measures, tones and words; false with the name that is missing. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the table of `entries`, else the set shown last, else one column of zeros; false with the reason. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const std::vector<DebugRsmEntry>& entries,
              double elapsed, bool still, std::string& failed);

private:
    /** The head word (the action) and the phase word of an entry; false with the reason. */
    bool words(const DebugRsmEntry& entry, std::string& head, std::string& phase_word, std::string& failed) const;
    /** The column at grid place `place`: its six cells, the SES cell red where `learning`; false with the reason. */
    bool column(UiPen& canvas, const ::godot::Transform2D& frame, const std::vector<std::string>& numbers_of,
                uint32_t place, bool learning, const UiTextPlace& text, std::string& failed);
    /** True where the entry's phase is GODOT_RSM_RED_PHASE: it learns now. */
    static bool learning(const DebugRsmEntry& entry);
    bool ring(UiPen& canvas, const ::godot::Transform2D& frame, uint32_t which, double share, double centre_y,
              const UiTextPlace& text, std::string& failed);
    /** The nine numbers of a column, as text, by row; false with the reason. */
    bool numbers(const DebugRsmEntry& entry, std::vector<std::string>& out, std::string& failed) const;
    bool number(const DebugRsmEntry& entry, uint32_t row, std::string& out, std::string& failed) const;
    bool grown(std::vector<UiTextBlock>& blocks, size_t count, bool data, std::string& failed);

    double x_ = 0.0;               // hud_debug_x
    double y_ = 0.0;               // hud_debug_y
    double cols_ = 0.0;            // hud_debug_rsm_cols
    double band_cols_ = 0.0;       // hud_debug_rsm_band_cols
    double cell_h_ = 0.0;          // hud_debug_rsm_cell_h
    double col_w_ = 0.0;           // hud_debug_rsm_col_w
    double col_gap_ = 0.0;         // hud_debug_rsm_col_gap
    double top_dy_ = 0.0;          // hud_debug_rsm_top_dy
    double bottom_dy_ = 0.0;       // hud_debug_rsm_bottom_dy
    double row_step_ = 0.0;        // hud_debug_rsm_row_step
    double pad_ = 0.0;             // hud_debug_rsm_pad
    double value_dx_ = 0.0;        // hud_debug_rsm_value_dx
    double size_ = 0.0;            // hud_debug_rsm_size
    double rule_gap_ = 0.0;        // hud_debug_rsm_rule_gap
    double rule_line_ = 0.0;       // hud_debug_rsm_rule_line
    double ring_x_ = 0.0;          // hud_debug_rsm_ring_x
    double ring_qnt_y_ = 0.0;      // hud_debug_rsm_ring_qnt_y
    double ring_tuc_y_ = 0.0;      // hud_debug_rsm_ring_tuc_y
    double ring_r_ = 0.0;          // hud_debug_rsm_ring_r
    double ring_line_ = 0.0;       // hud_debug_rsm_ring_line
    double ring_word_dy_ = 0.0;    // hud_debug_rsm_ring_word_dy
    double ring_pct_dy_ = 0.0;     // hud_debug_rsm_ring_pct_dy
    double ring_pct_size_ = 0.0;   // hud_debug_rsm_ring_pct_size

    ::godot::Color ink_;           // DEBUG_INK
    ::godot::Color ink_on_;        // DEBUG_INK_ON
    ::godot::Color edge_;          // DEBUG_LINE
    ::godot::Color field_;         // DEBUG_FIELD: the grid lines
    ::godot::Color cell_;          // DEBUG_CELL: a cell
    ::godot::Color run_;           // DEBUG_RUN: the SES cell of an entry that learns now

    std::vector<std::string> row_words_;     // texts.hud.debug_rsm.<row>, by row of a column: ses acd tuc dcd qnt tul que lck exp
    std::vector<std::string> phase_words_;   // texts.hud.debug_rsm.phase.<n>
    std::vector<std::string> act_words_;     // texts.hud.debug_rsm.act.<code> by debug_rsm_action_slot; empty where the theme has none
    std::vector<HudRun> percent_;            // texts.hud.debug_rsm.percent
    std::vector<HudRun> cell_word_;          // texts.hud.debug_rsm.cell: "{key}: {value}"
    std::string idle_word_;                  // texts.hud.debug_rsm.idle: head and phase of the column of zeros
    std::vector<DebugRsmEntry> held_;        // the set shown last, kept while the port names no entry

    const UiTheme* theme_ = nullptr;         // the theme bind() read, to bind a block a longer table adds
    UiRing ring_;                            // the drawer of the rings QNT and TUC
    std::vector<UiTextBlock> row_labels_;    // the word of each row, shared by every column
    std::vector<UiTextBlock> heads_;         // the head of each column
    std::vector<UiTextBlock> phases_;        // the phase word of each column
    std::vector<UiTextBlock> values_;        // the number of each cell, by column then row
    std::vector<UiTextBlock> ring_words_;    // the word of each ring
    std::vector<UiTextBlock> ring_values_;   // the percent of each ring
};

}  // namespace ase::adp::godot
