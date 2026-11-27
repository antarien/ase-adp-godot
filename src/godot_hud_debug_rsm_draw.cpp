/**
 * ASE GODOT ADAPTER - HUD DEBUG RSM TABLE DRAWING
 *
 * @file        godot_hud_debug_rsm_draw.cpp
 * @brief       The RSM table of the debug layer drawn: a column per learning entry, the table standing always
 * @description MIG_ASE_SKILL_RSM step B4, see godot_hud_debug_rsm_draw.hpp.
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

#include <ase/adp/godot/godot_hud_debug_rsm_draw.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_hud_ui_text_place.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>
#include <ase/adp/godot/hud/ring/types.hpp>
#include <ase/adp/godot/rsm/types.hpp>

#include <cmath>
#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

namespace {

constexpr uint32_t kPercentRow = 6u;                         // QUE: the fill of the queue, written as per cent
constexpr uint32_t kProgressRow = 9u;                        // PRG: the progress toward the next plateau, written as per cent
constexpr uint32_t kRingRows[4] = {3u, 1u, 4u, 2u};          // the row of a column each ring stands for: DCD, ACD, QNT, TUC
constexpr uint32_t kClockRows = (1u << 0) | (1u << 2) | (1u << 5) | (1u << 7) | (1u << 11);  // SES, TUC, TUL, LCK, MSR: HH:MM:SS
constexpr uint32_t kRateRows = 1u << 10;                     // VEL: GODOT_RSM_RATE_DECIMALS places after the point

/** The point (x, y) in mockup units. */
::godot::Vector2 at(double x, double y) {
    return ::godot::Vector2(static_cast<float>(x), static_cast<float>(y));
}

/** The baseline of grid row `row` counted under `top`: the six rows of the first band, then the rows of the second band,
 *  which stand under the divider a gap lower (GODOT_RSM_ROWS_ABOVE_RULE, the gap hud_debug_rsm_rule_gap). */
double row_y(double top, double step, double gap, uint32_t row) {
    return top + static_cast<double>(row + 1u) * step + (row >= GODOT_RSM_ROWS_ABOVE_RULE ? gap : 0.0);
}

/** The index of `code` in a list of `count` words starting at `first`; false where it is no code of the list. */
bool word_index(double code, uint32_t first, uint32_t count, uint32_t& out) {
    if (!std::isfinite(code) || code < static_cast<double>(first) || code >= static_cast<double>(first + count)) {
        return false;
    }
    out = static_cast<uint32_t>(code) - first;
    return true;
}

}  // namespace

bool DebugRsmTable::bind(const UiTheme& theme, std::string& failed) {
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_debug_x", &x_},
             {"hud_debug_y", &y_},
             {"hud_debug_rsm_cols", &cols_},
             {"hud_debug_rsm_band_cols", &band_cols_},
             {"hud_debug_rsm_cell_h", &cell_h_},
             {"hud_debug_rsm_col_w", &col_w_},
             {"hud_debug_rsm_col_gap", &col_gap_},
             {"hud_debug_rsm_top_dy", &top_dy_},
             {"hud_debug_rsm_bottom_dy", &bottom_dy_},
             {"hud_debug_rsm_row_step", &row_step_},
             {"hud_debug_rsm_pad", &pad_},
             {"hud_debug_rsm_value_dx", &value_dx_},
             {"hud_debug_rsm_size", &size_},
             {"hud_debug_rsm_rule_gap", &rule_gap_},
             {"hud_debug_rsm_rule_line", &rule_line_},
             {"hud_debug_rsm_ring_x", &ring_x_},
             {"hud_debug_rsm_ring_qnt_y", &ring_qnt_y_},
             {"hud_debug_rsm_ring_tuc_y", &ring_tuc_y_},
             {"hud_debug_rsm_ring_r", &ring_r_},
             {"hud_debug_rsm_ring_line", &ring_line_},
             {"hud_debug_rsm_ring_word_dy", &ring_word_dy_},
             {"hud_debug_rsm_ring_pct_dy", &ring_pct_dy_},
             {"hud_debug_rsm_ring_pct_size", &ring_pct_size_}}) {
        if (!theme.measure(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{
             {"DEBUG_INK", &ink_}, {"DEBUG_INK_ON", &ink_on_}, {"DEBUG_LINE", &edge_}, {"DEBUG_FIELD", &field_},
             {"DEBUG_CELL", &cell_}, {"DEBUG_RUN", &run_}}) {
        if (!theme.color(slot.first, *slot.second)) {
            failed = slot.first;
            return false;
        }
    }
    row_words_.assign(GODOT_RSM_VALUE_ROWS, std::string());
    uint32_t row = 0;
    for (const char* name : {"ses", "acd", "tuc", "dcd", "qnt", "tul", "que", "lck", "exp", "prg", "vel", "msr", "vol"}) {
        if (!plain_word(theme, std::string("debug_rsm.") + name, row_words_[row++], failed)) {
            return false;
        }
    }
    phase_words_.assign(GODOT_RSM_PHASES, std::string());
    for (uint32_t phase = 0; phase < GODOT_RSM_PHASES; ++phase) {
        if (!plain_word(theme, "debug_rsm.phase." + std::to_string(phase), phase_words_[phase], failed)) {
            return false;
        }
    }
    // The head of a column is the word of the entry's action code (skill_id = BDI_CONST_ACTION_*). The span holds codes
    // no constant carries (154, 155): a key the theme lacks leaves its word empty, and a column of that code names the
    // missing key when it is drawn; a key the theme has must be plain text.
    act_words_.assign(GODOT_RSM_ACTION_LAST - GODOT_RSM_ACTION_FIRST + 1u, std::string());
    for (uint32_t act = GODOT_RSM_ACTION_FIRST; act <= GODOT_RSM_ACTION_LAST; ++act) {
        const std::string key = "debug_rsm.act." + std::to_string(act);
        std::vector<HudRun> runs;
        if (theme.words(key, runs) && !plain_word(theme, key, act_words_[act - GODOT_RSM_ACTION_FIRST], failed)) {
            return false;
        }
    }
    if (!plain_word(theme, "debug_rsm.idle", idle_word_, failed)) {
        return false;
    }
    if (!theme.words("debug_rsm.percent", percent_)) {
        failed = "texts.hud.debug_rsm.percent";
        return false;
    }
    if (!theme.words("debug_rsm.cell", cell_word_)) {
        failed = "texts.hud.debug_rsm.cell";
        return false;
    }
    if (band_cols_ < 1.0 || cols_ < 1.0) {
        failed = "hud_debug_rsm_band_cols and hud_debug_rsm_cols: the grid needs at least one column";
        return false;
    }
    theme_ = &theme;
    row_labels_.clear();
    heads_.clear();
    phases_.clear();
    values_.clear();
    ring_words_.clear();
    ring_values_.clear();
    return ring_.bind(theme, failed);
}

bool DebugRsmTable::grown(std::vector<UiTextBlock>& blocks, size_t count, bool data, std::string& failed) {
    if (theme_ == nullptr) {
        failed = "debug rsm: a text block was asked for before bind() read the theme";
        return false;
    }
    for (size_t index = blocks.size(); index < count; ++index) {
        blocks.emplace_back();
        if (!(data ? blocks.back().bind_data(*theme_, failed) : blocks.back().bind_label(*theme_, failed))) {
            blocks.pop_back();
            return false;
        }
    }
    return true;
}

bool DebugRsmTable::number(const DebugRsmEntry& entry, uint32_t row, std::string& out, std::string& failed) const {
    // PRG comes from the server as 0 to 100; the per cent row takes a share of 0 to 1 like QUE.
    const double values[GODOT_RSM_VALUE_ROWS] = {entry.ses,   entry.acd,  entry.tuc,
                                                 entry.dcd,   entry.qnt_left, entry.tul,
                                                 entry.que_fill, entry.lck, entry.dgexp,
                                                 entry.prg / GODOT_RSM_PERCENT, entry.vel, entry.msr,
                                                 entry.vol};
    if (row >= GODOT_RSM_VALUE_ROWS) {
        failed = "debug rsm: a row of a column beyond the last";
        return false;
    }
    if (row == kPercentRow || row == kProgressRow) {
        out = "-";
        std::vector<HudRun> percent = percent_;
        return !std::isfinite(values[row]) ||
               (fill_words(percent, {HudPortValue{"value", hud_percent(values[row])}}, failed) && word_text(percent, out, failed));
    }
    if (((kRateRows >> row) & 1u) != 0u) {
        out = std::isfinite(values[row]) ? hud_decimals(values[row], GODOT_RSM_RATE_DECIMALS) : std::string("-");
        return true;
    }
    out = ((kClockRows >> row) & 1u) != 0u ? debug_rsm_clock(values[row]) : debug_rsm_whole(values[row]);
    return true;
}

bool DebugRsmTable::numbers(const DebugRsmEntry& entry, std::vector<std::string>& out, std::string& failed) const {
    out.assign(GODOT_RSM_VALUE_ROWS, std::string());
    for (uint32_t row = 0; row < GODOT_RSM_VALUE_ROWS; ++row) {
        if (!number(entry, row, out[row], failed)) {
            return false;
        }
    }
    return true;
}

bool DebugRsmTable::words(const DebugRsmEntry& entry, std::string& head, std::string& phase_word,
                          std::string& failed) const {
    uint32_t act = 0;
    uint32_t phase = 0;
    if (!debug_rsm_action_slot(entry.action, act) || act_words_[act].empty()) {
        failed = "debug rsm: the action " + debug_rsm_whole(entry.action) + " of entry " + std::to_string(entry.id) +
                 " has no word (texts.hud.debug_rsm.act.<code>, BDI_CONST_ACTION_*)";
        return false;
    }
    if (!word_index(entry.phase, 0u, GODOT_RSM_PHASES, phase)) {
        failed = "debug rsm: the phase of entry " + std::to_string(entry.id) + " has no word (texts.hud.debug_rsm.phase.<n>)";
        return false;
    }
    head = act_words_[act];
    phase_word = phase_words_[phase];
    return true;
}

bool DebugRsmTable::learning(const DebugRsmEntry& entry) {
    uint32_t phase = 0;
    return word_index(entry.phase, 0u, GODOT_RSM_PHASES, phase) && phase == GODOT_RSM_RED_PHASE;
}

bool DebugRsmTable::column(UiPen& canvas, const ::godot::Transform2D& frame, const std::vector<std::string>& numbers_of,
                           uint32_t place, bool learning, const UiTextPlace& text, std::string& failed) {
    // THE GRID OF THE PROTOTYPE: the column `place` stands in band place / band_cols at slot place % band_cols; its six
    // cells touch, so their outlines are the lines of the grid; the second band stands under the divider (row_y)
    const uint32_t band_cols = static_cast<uint32_t>(band_cols_);
    const uint32_t band = place / band_cols;
    const double left = x_ + static_cast<double>(place % band_cols) * (col_w_ + col_gap_);
    for (uint32_t row = 0; row < GODOT_RSM_GRID_ROWS; ++row) {
        const double base = row_y(y_ - row_step_, row_step_, rule_gap_, band * GODOT_RSM_GRID_ROWS + row);
        const ::godot::Rect2 box(static_cast<float>(left), static_cast<float>(base + top_dy_), static_cast<float>(col_w_),
                                 static_cast<float>(cell_h_));
        canvas.draw_set_transform_matrix(frame);
        canvas.draw_rect(box, learning && row == 0u ? run_ : cell_, true);
        canvas.draw_rect(box, field_, false, static_cast<float>(rule_line_));
        canvas.draw_set_transform_matrix(::godot::Transform2D());

        // "KEY: value" as the prototype's TimerView writes it, the form a word of the theme
        std::vector<HudRun> runs = cell_word_;
        if (!fill_words(runs, {HudPortValue{"key", row_words_[row]}, HudPortValue{"value", numbers_of[row]}}, failed)) {
            return false;
        }
        UiTextPlace value = text;
        value.at = at(left + pad_, base);
        value.color = ink_on_;
        UiTextBlock& block = values_[place * GODOT_RSM_GRID_ROWS + row];
        if (!block.set(runs, failed)) {
            return false;
        }
        block.draw(canvas, frame, value);
    }
    return true;
}

bool DebugRsmTable::ring(UiPen& canvas, const ::godot::Transform2D& frame, uint32_t which, double share,
                         double centre_y, const UiTextPlace& text, std::string& failed) {
    const ::godot::Vector2 centre = at(ring_x_, centre_y);
    canvas.draw_set_transform_matrix(frame);
    ring_.circle(canvas, centre, ring_r_, edge_, ring_line_, 1.0);
    UiArcStyle style;
    style.opacity = 1.0;
    style.round_cap = false;
    style.drawn = 1.0;
    ring_.arc(canvas, centre, ring_r_, 0.0, share * GODOT_RING_TURN_DEG, ink_, ring_line_, style);
    canvas.draw_set_transform_matrix(::godot::Transform2D());

    // The word of the ring above it: the word of the row of its timer, DCD row 3, ACD row 1, QNT row 4, TUC row 2.
    UiTextPlace word = text;
    word.at = at(ring_x_, centre_y + ring_word_dy_);
    word.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    if (!ring_words_[which].set(row_words_[kRingRows[which]], failed)) {
        return false;
    }
    ring_words_[which].draw(canvas, frame, word);

    std::vector<HudRun> percent = percent_;
    if (!fill_words(percent, {HudPortValue{"value", hud_percent(share)}}, failed) || !ring_values_[which].set(percent, failed)) {
        return false;
    }
    UiTextPlace value = text;
    value.at = at(ring_x_, centre_y + ring_pct_dy_);
    value.size = ring_pct_size_;
    value.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    value.color = ink_on_;
    ring_values_[which].draw(canvas, frame, value);
    return true;
}

bool DebugRsmTable::draw(UiPen& canvas, const ::godot::Transform2D& frame, const std::vector<DebugRsmEntry>& entries,
                         double elapsed, bool still, std::string& failed) {
    const size_t cols = static_cast<size_t>(cols_);
    if (!grown(values_, cols * GODOT_RSM_GRID_ROWS, true, failed)) {
        return false;
    }
    UiTextPlace text;
    text.size = size_;
    text.anchor = GODOT_TEXT_ANCHOR_START;
    text.color = ink_;
    text.opacity = 1.0;
    text.elapsed = elapsed;
    text.still = still;
    // THE TABLE GROWS WITH THE SKILLS AND STANDS ALWAYS (operator 2026-11-25): one column per learning entry of the
    // port, the newest left, a new column as soon as a further skill is used; a frame whose port names no entry keeps
    // the set shown last with its last values; the tab carries the timer grid alone, no ring
    std::vector<DebugRsmEntry> shown;
    debug_rsm_shown(entries, static_cast<uint32_t>(cols), held_, shown);
    std::vector<std::string> numbers_of;
    if (shown.empty()) {
        // before the first learning order: one column of zeros
        return numbers(DebugRsmEntry{}, numbers_of, failed) && column(canvas, frame, numbers_of, 0u, false, text, failed);
    }
    for (size_t place = 0; place < shown.size(); ++place) {
        if (!numbers(shown[place], numbers_of, failed) ||
            !column(canvas, frame, numbers_of, static_cast<uint32_t>(place), learning(shown[place]), text, failed)) {
            return false;
        }
    }
    return true;
}

}  // namespace ase::adp::godot
