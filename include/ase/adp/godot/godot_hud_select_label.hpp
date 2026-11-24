#pragma once

/**
 * ASE GODOT ADAPTER - HUD SELECT LABEL
 *
 * @file        godot_hud_select_label.hpp
 * @brief       The reading of a single tap: what the selected thing is, at the depth of perception
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD row L01, block SelectLabel (the mockup's selLabel).
 *
 *              THE LEADER. From the top of the thing's box (its centre, hud_select_anchor_dy above its top) a line
 *              in the item's tone runs to the elbow hud_select_elbow_dx to the side and hud_select_rise up, never
 *              above hud_select_min_y, then hud_select_run along past the text gap hud_select_text_gap. A box whose
 *              centre stands left of hud_select_split_x reads to the right (texts anchored at their start), every
 *              other to the left (anchored at their end). The line draws in as UiLeaderLine draws.
 *
 *              THE WORDS. The name items.<id>.name in class tb at hud_select_title_size in the tone,
 *              hud_select_title_dy above the elbow. From hud.perception.levels[1] on, the item widget's reading
 *              (ItemWidget::reading: texts.hud.item_size, item_volume, item_count) in class d at
 *              hud_select_facts_size in OVERLAY_INK, hud_select_facts_dy below the elbow, the next line
 *              hud_select_facts_step lower. Then the hint in class t at hud_select_hint_size in OVERLAY_INK_SOFT:
 *              texts.hud.tap_open on the linen pouch, texts.hud.tap_use on the thing in the hand, else
 *              texts.hud.tap_hand; hud_select_hint_step lower, from hud.perception.levels[2] on, the asset the
 *              thing is drawn from, or its phase-04 note where it has none, in class d at hud_select_asset_size
 *              in OVERLAY_INK_FAINT.
 *
 *              FULL FROM THE FRAME. The thing, its facts and the box it was drawn in are the frame's; the facts are
 *              state of ase-inventory on flora.carry.v1, written from SHA by VivariumCarryBridge until the port
 *              delivers (gap N0451, VivariumView::open_gap). The widget of each item is bound on first use from the
 *              class hud.items.<id>.widget names.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0451 (flora.carry.v1), carrier N0448
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_item_widget.hpp>
#include <ase/adp/godot/godot_hud_ui_leader_line.hpp>
#include <ase/adp/godot/godot_hud_ui_text_class.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** The selected thing this frame (S.sel) and where it was drawn. */
struct SelectLabelFrame {
    std::string id;              // o.id: the item, the key of hud.items
    double length_cm = 0.0;      // D.len
    double kg = 0.0;             // D.kg: one piece
    double weight_kg = 0.0;      // wt(o)
    double litres = 0.0;         // D.l: one piece
    int64_t count = 0;           // o.n
    bool stack = false;          // D.stack
    bool opens = false;          // o.id === 'bolsa': the thing is the linen pouch, a double tap opens it
    bool held = false;           // o.c === 'hand': the thing is in the hand
    ::godot::Rect2 box;          // POS[o.key]: its box in frame units
    uint32_t level = 0;          // the depth of perception
    double elapsed = 0.0;        // the seconds since the reading appeared
    bool still = false;          // reduced motion: every animation at its end
};

/** The reading of a single tap. */
class SelectLabel {
public:
    /** Binds measures, colours, words and levels; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The leader and the words of the selected thing; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const SelectLabelFrame& now, std::string& failed);

private:
    /** The widget of the item `id`, bound on first use from its class; nullptr with the reason in `failed`. */
    const ItemWidget* widget(const std::string& id, std::string& failed);

    /** Draws `runs` into `block` at `place`; false with the reason in `failed`. */
    static bool say(UiPen& canvas, const ::godot::Transform2D& frame, UiTextBlock& block,
                    const std::vector<HudRun>& runs, const UiTextPlace& place, std::string& failed);

    const UiTheme* theme_ = nullptr;
    double split_x_ = 0.0, elbow_dx_ = 0.0, rise_ = 0.0, min_y_ = 0.0;        // hud_select_*
    double anchor_dy_ = 0.0, text_gap_ = 0.0, run_ = 0.0;                     // hud_select_*
    double title_dy_ = 0.0, title_size_ = 0.0;                                // hud_select_title_*
    double facts_dy_ = 0.0, facts_size_ = 0.0, facts_step_ = 0.0;             // hud_select_facts_*
    double hint_size_ = 0.0, hint_step_ = 0.0, asset_size_ = 0.0;             // hud_select_hint_*, _asset_size
    uint32_t level_facts_ = 0;                                                // hud.perception.levels[1]
    uint32_t level_asset_ = 0;                                                // hud.perception.levels[2]
    ::godot::Color ink_, soft_, faint_;                                       // OVERLAY_INK, _SOFT, _FAINT
    std::vector<HudRun> tap_open_, tap_use_, tap_hand_;                       // texts.hud.tap_*
    UiTextBlock title_text_;   // class tb: the name
    UiTextBlock label_text_;   // class t: the hint
    UiTextBlock data_text_;    // class d: the reading and the asset
    UiLeaderLine line_;        // the leader
    std::vector<ItemWidget> widgets_;  // one per item, bound on first use
};

}  // namespace ase::adp::godot
