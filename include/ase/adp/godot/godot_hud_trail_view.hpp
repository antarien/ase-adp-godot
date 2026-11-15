#pragma once

/**
 * ASE GODOT ADAPTER - HUD TRAIL VIEW
 *
 * @file        godot_hud_trail_view.hpp
 * @brief       The trail on arrival: how long the keeper was away and the last events, down to the play chip
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, building block TrailView (the mockup's arrive()),
 *              drawn in the mode arrive, which the view sets itself when it starts (return to the garden,
 *              usecase:100390).
 *
 *              THE DIM. The world dims from hud_arrive_dim_y, hud_arrive_dim_h high, stretched to the surface the
 *              HUD sets in `cover` (hud_cover_dim), in OVERLAY_SCRIM_TRAIL. Over it texts.hud.arrive_title (class tb, hud_arrive_title_size) at
 *              hud_arrive_x, hud_arrive_title_y, and texts.hud.arrive_span with the days away, the LUNBREX and
 *              the first and last TUMBREX (class d, hud_arrive_span_size, OVERLAY_INK_SOFT, after
 *              hud_arrive_span_delay).
 *
 *              THE SPINE. The last hud_arrive_events events of the port, in its order, one row each at
 *              hud_arrive_row_y plus i * hud_arrive_row_step. A line in OVERLAY_INK_FAINT runs down hud_arrive_x
 *              from hud_arrive_spine_top to the last row plus hud_arrive_spine_tail and draws itself in. Row i
 *              fades in after hud_arrive_row_delay plus i * hud_arrive_row_stagger, in the tone of its domain:
 *              the newest row a pulse of hud_arrive_pulse_radius, every row a dot of hud_arrive_dot_radius and a
 *              tick from hud_arrive_tick_from to hud_arrive_tick_to. At hud_arrive_text_x: the date
 *              texts.hud.arrive_date (class d, hud_arrive_date_size, OVERLAY_INK_SOFT, hud_arrive_date_dy), the
 *              title texts.hud.trail.<kind>.title (class tb, hud_arrive_event_size, the domain tone for an alert,
 *              else OVERLAY_INK, hud_arrive_event_dy), the line texts.hud.trail.<kind>.line (class t,
 *              hud_arrive_detail_size, OVERLAY_INK_SOFT, hud_arrive_detail_dy), and from perception
 *              levels[2] the case the event stands on (class d, hud_arrive_case_size, OVERLAY_INK_FAINT,
 *              hud_arrive_case_dy). The tap target of a row is hud_arrive_hit_w by hud_arrive_hit_h from
 *              hud_arrive_hit_x, hud_arrive_hit_dy above the row; it opens the sign of the event, or rests
 *              where the event names none.
 *
 *              THE PLAY CHIP. A hexagon of hud_arrive_play_radius at hud_arrive_play_x, hud_arrive_play_y with
 *              the glyph texts.hud.arrive_play_glyph in OVERLAY_INK, after hud_arrive_play_delay, and under it
 *              texts.hud.arrive_play (class t, hud_arrive_play_label_size, middle) at hud_arrive_play_label_y
 *              after hud_arrive_play_label_delay. A tap rests: the view goes back to the tree.
 *
 *              BRIDGE GAP N0450. The events and the span are the records of flora.trail.v1 (kind, tone, lun,
 *              tum, values, case_id, sign, alert per event; days, lun, from, to of the span). The unit draws the
 *              title, the span, the spine, every row and the play chip in full from TrailViewFrame and never
 *              asks whether the port filed them: while it files none VivariumTrailBridge fills the frame with
 *              the mockup's values from SHA, and VivariumView::fill_hud logs the gap with the missing fields once
 *              on the edge.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:100390 - return to the garden
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    N0450 (flora.trail.v1: the past events with date, domain, words and case), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_hex_chip.hpp>
#include <ase/adp/godot/godot_hud_ui_leader_line.hpp>
#include <ase/adp/godot/godot_hud_ui_motion.hpp>
#include <ase/adp/godot/godot_hud_ui_ring.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>
#include <ase/adp/godot/godot_hud_ui_word_fill.hpp>

#include <ase/adp/godot/godot_hud_ui_pen.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One past event of flora.trail.v1. */
struct TrailEvent {
    uint32_t lun = 0;                   // the LUNBREX of its day
    uint32_t tum = 0;                   // the TUMBREX of its day
    std::string kind;                   // texts.hud.trail.<kind>: soak, flush, sway, dry, wilt
    std::string tone;                   // the domain whose tone it is drawn in (DOMAIN_<TONE>)
    bool alert = false;                 // the title takes the domain tone instead of OVERLAY_INK
    std::vector<HudPortValue> values;   // the port values its title and line name
    std::string case_id;                // the case the event stands on, as the port writes it
    uint32_t sign = UINT32_MAX;         // index into HudContextSpec::signs it opens; UINT32_MAX rests
};

/** What the trail shows this frame. */
struct TrailViewFrame {
    std::vector<TrailEvent> events;     // the past events, oldest first
    uint32_t days = 0;                  // the days away
    uint32_t lun = 0;                   // the LUNBREX of the span
    uint32_t from = 0;                  // the first TUMBREX of the span
    uint32_t to = 0;                    // the last TUMBREX of the span
    uint32_t level = 0;                 // the perception level (hud.perception.levels)
    double elapsed = 0.0;               // seconds since the mode arrive opened
    bool still = false;                 // every motion held
    HudCover cover;                     // the surface the dim reaches, set by the HUD
};

/** The trail on arrival. */
class TrailView {
public:
    /** Binds measures, colours, words, levels, the line, the ring, the chip and the fade; false with the name. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the dim, the title, the span, the rows and the play chip; false with the reason. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const TrailViewFrame& now,
              std::string& failed);

    /**
     * What `point` (in mockup units) hits: true on a row or on the play chip, with the sign the row opens in
     * `sign`, or UINT32_MAX to rest. False where it hits neither.
     */
    bool hit(const ::godot::Vector2& point, uint32_t& sign) const;

    /** The square around the play chip (mockup units) as it stood in the last draw: its middle is the chip's
     *  centre, where hit() rests, and it holds the hexagon of hud_arrive_play_radius; true in `hidden` while
     *  the chip did not stand. */
    void play_place(::godot::Rect2& place, bool& hidden) const;

    /** The target of row `index` drawn last (mockup units, the box hit() asks) and the sign it opens; false past
     *  the last row drawn. */
    bool row_place(size_t index, ::godot::Rect2& box, uint32_t& sign) const;

private:
    /** One row drawn last, for hit(). */
    struct Drawn {
        ::godot::Rect2 box;
        uint32_t sign = UINT32_MAX;
    };

    /** The texts of one row. */
    struct Row {
        UiTextBlock date;    // class d
        UiTextBlock title;   // class tb
        UiTextBlock line;    // class t
        UiTextBlock cased;   // class d
    };

    /** The word `key` with `values` filled in, set on `block`. */
    bool set_word(UiTextBlock& block, const std::string& key, const std::vector<HudPortValue>& values,
                  std::string& failed) const;

    /** Draws row `i` of `event` at `y`. */
    bool draw_row(UiPen& canvas, const ::godot::Transform2D& frame, const TrailViewFrame& now,
                  const TrailEvent& event, size_t i, bool newest, double y, std::string& failed);

    /** Draws a text at `x`, `y` in `size` and `color`, faded in after `delay`. */
    void put(UiTextBlock& block, UiPen& canvas, const ::godot::Transform2D& frame,
             const TrailViewFrame& now, double x, double y, double size, const ::godot::Color& color,
             double anchor, double delay) const;

    const UiTheme* theme_ = nullptr;
    double dim_y_ = 0.0;            // hud_arrive_dim_y
    double dim_h_ = 0.0;            // hud_arrive_dim_h
    double x_ = 0.0;                // hud_arrive_x
    double title_y_ = 0.0;          // hud_arrive_title_y
    double title_size_ = 0.0;       // hud_arrive_title_size
    double span_y_ = 0.0;           // hud_arrive_span_y
    double span_size_ = 0.0;        // hud_arrive_span_size
    double span_delay_ = 0.0;       // hud_arrive_span_delay
    double spine_top_ = 0.0;        // hud_arrive_spine_top
    double spine_tail_ = 0.0;       // hud_arrive_spine_tail
    double events_ = 0.0;           // hud_arrive_events
    double row_y_ = 0.0;            // hud_arrive_row_y
    double row_step_ = 0.0;         // hud_arrive_row_step
    double row_delay_ = 0.0;        // hud_arrive_row_delay
    double row_stagger_ = 0.0;      // hud_arrive_row_stagger
    double pulse_radius_ = 0.0;     // hud_arrive_pulse_radius
    double dot_radius_ = 0.0;       // hud_arrive_dot_radius
    double tick_from_ = 0.0;        // hud_arrive_tick_from
    double tick_to_ = 0.0;          // hud_arrive_tick_to
    double text_x_ = 0.0;           // hud_arrive_text_x
    double date_dy_ = 0.0;          // hud_arrive_date_dy
    double date_size_ = 0.0;        // hud_arrive_date_size
    double event_dy_ = 0.0;         // hud_arrive_event_dy
    double event_size_ = 0.0;       // hud_arrive_event_size
    double detail_dy_ = 0.0;        // hud_arrive_detail_dy
    double detail_size_ = 0.0;      // hud_arrive_detail_size
    double case_dy_ = 0.0;          // hud_arrive_case_dy
    double case_size_ = 0.0;        // hud_arrive_case_size
    double hit_x_ = 0.0;            // hud_arrive_hit_x
    double hit_dy_ = 0.0;           // hud_arrive_hit_dy
    double hit_w_ = 0.0;            // hud_arrive_hit_w
    double hit_h_ = 0.0;            // hud_arrive_hit_h
    double play_x_ = 0.0;           // hud_arrive_play_x
    double play_y_ = 0.0;           // hud_arrive_play_y
    double play_radius_ = 0.0;      // hud_arrive_play_radius
    double play_delay_ = 0.0;       // hud_arrive_play_delay
    double play_label_y_ = 0.0;     // hud_arrive_play_label_y
    double play_label_size_ = 0.0;  // hud_arrive_play_label_size
    double play_label_delay_ = 0.0; // hud_arrive_play_label_delay
    double pulse_line_ = 0.0;       // hud_line
    uint32_t level_case_ = 0;       // hud.perception.levels[2]: the case line

    ::godot::Color scrim_;          // OVERLAY_SCRIM_TRAIL
    ::godot::Color ink_;            // OVERLAY_INK
    ::godot::Color soft_;           // OVERLAY_INK_SOFT
    ::godot::Color faint_;          // OVERLAY_INK_FAINT

    UiTextBlock title_;             // texts.hud.arrive_title, class tb
    UiTextBlock span_;              // texts.hud.arrive_span, class d
    UiTextBlock play_label_;        // texts.hud.arrive_play, class t
    std::vector<Row> rows_;         // hud_arrive_events rows
    UiHexChip play_;                // the chip with texts.hud.arrive_play_glyph
    UiHexChipPlace play_place_;     // where it stood last
    UiLeaderLine spine_;            // the spine
    UiMotion fade_;                 // hud_anim_fade
    UiRing ring_;

    std::vector<Drawn> drawn_;      // the rows of the last draw
    bool play_drawn_ = false;       // the play chip stood in the last draw
};

}  // namespace ase::adp::godot
