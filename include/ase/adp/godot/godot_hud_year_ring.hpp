#pragma once

/**
 * ASE GODOT ADAPTER - HUD YEAR RING
 *
 * @file        godot_hud_year_ring.hpp
 * @brief       The year ring of the calendar sheet: the LUN of the year, the seasons, today, past events, the next stage
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, block YearRing (the outer ring of the mockup's
 *              skySheet()). Centre (hud_year_x, hud_year_y), radius hud_year_radius; an angle is degrees from
 *              the top, clockwise, and LUN m begins at (m - 1) turns of a LUN, a LUN being a full turn over
 *              the LUN of a year.
 *
 *              THE SEASONS. One arc per season of texts.hud.seasons.* from the turn of its first LUN to the
 *              turn of the next season's first LUN, hud_year_season_gap outside the ring, cut by
 *              hud_year_season_trim at each end, in hud_year_season_width and the season's colour SEASON_*;
 *              each grows from its start (UiRing's draw) after hud_year_season_stagger times its index. Its
 *              word in class t at the middle of the arc, hud_year_season_label_gap outside the ring, size
 *              hud_year_season_label_size, centred, fading in after hud_year_season_label_delay; from
 *              perception levels[1] on texts.hud.light with the season's hours of light in class d
 *              hud_year_light_dy below it, size hud_year_light_size, in OVERLAY_INK_SOFT, fading in after
 *              hud_year_light_delay.
 *
 *              THE MARKS. The mockup's ring carries hud_year_marks numbered marks spread evenly round the year
 *              (m.html 852 to 856, 13); the LUN of a year is the clock record's and runs far past it (149 under
 *              CAL_CONST_MONTHS_PER_YEAR), so a tick and a number per LUN would heap into an unreadable band.
 *              Mark k of hud_year_marks stands at the start of the first LUN that begins at or after k parts of
 *              the turn, so every number names a real LUN and the marks keep the mockup's spacing; a year of
 *              fewer LUN than marks has one mark per LUN. Each mark is a tick from hud_year_tick_inner to
 *              hud_year_tick_outer off the ring in OVERLAY_INK_FAINT at hud_line_hair; its LUN number in class
 *              d half a mark's stretch further on, hud_year_number_inset inside the ring and hud_year_number_dy
 *              lower, size hud_year_number_size, centred, in DOMAIN_TIME for the mark whose stretch holds the
 *              current LUN and OVERLAY_INK_SOFT for every other. That stretch, from its mark to the next, is an
 *              arc hud_year_lun_inset inside the ring, cut by hud_year_lun_trim, in hud_year_lun_width and
 *              DOMAIN_TIME at hud_year_lun_alpha.
 *
 *              TODAY. A line at today's share of the year from hud_year_today_inner to hud_year_today_outer
 *              off the ring in DOMAIN_TIME at hud_year_today_line.
 *
 *              THE TRAIL. One dot of hud_year_event_radius per past event, at its share of the year,
 *              hud_year_event_inset inside the ring, in the colour of its domain tone (DOMAIN_<TONE>).
 *
 *              THE NEXT STAGE. A circle of hud_year_next_radius at the share of the year on which the plant
 *              reaches its next stage, hud_year_next_gap outside the ring, stroked DOMAIN_LIFE at
 *              hud_year_next_line; from perception levels[1] on the stage's word texts.hud.stages.<n> in
 *              class t, hud_year_next_label_dx right and hud_year_next_label_dy below it, size
 *              hud_year_next_label_size, in DOMAIN_LIFE. A plant that is dead or at its last stage has no
 *              next stage, and the circle is not drawn.
 *
 *              THE PLACE OF A DAY. The unit never turns a date into a day of the year: that is the clock
 *              logic of ase-calendar, and the adapter translates. Each day stands where the module puts it,
 *              as its share of the year (the start of the day over the year, 0 to 1), and the ring turns the
 *              share into an angle. The LUN of a year and the current LUN are the clock record's
 *              (VivariumClockView months_per_year, month); the ring, the months and the seasons are always
 *              drawn in full from the frame, and the ring never asks whether a port delivered.
 *
 *              BRIDGE GAP N0460. The first LUN of each season and its hours of light come from the four
 *              records of flora.season.v1 (season, start_lun from CAL_CONST_SEASON_*_START, light_h from
 *              CAL_CONST_SSN_DAYLIGHT_*), today's share of the year from the day record flora.day.v1
 *              (year_share). GodotHostResourceManager::read_snapshot reads them into VivariumClockView, the
 *              bridge fills season_starts, season_light and year_share; while a port files no record the bridge
 *              writes the mockup's values from hud_bridge_clock_* of SHA and logs the gap once on its edge
 *              (godot_vivarium_hud_bridge.cpp, bridge_clock). BRIDGE GAP N0450.
 *              The past events with their share of the year and their domain are flora.trail.v1; while it files
 *              none VivariumTrailBridge::ring writes the mockup's events. BRIDGE GAP N0454. The share of the year
 *              of the next stage is the forecast of ase-phytomer on flora.patch.v1; while it files none
 *              VivariumView::bridge_patch writes the mockup's. Each part is drawn in full from its frame and
 *              the ring never asks who filled it; the gaps are logged once on the edge by VivariumView.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:5606 - observe the sky
 * @actor       Sky Watch Guide (SKYWATGUI)
 * @register    N0460 (flora.season.v1: seasons, light hours; flora.day.v1: share of the year), N0450 (flora.trail.v1), N0454 (flora.patch.v1: next stage), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

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
#include <vector>

namespace ase::adp::godot {

/** One past event on the ring. */
struct YearRingEvent {
    double year_share = 0.0;  // the share of the year of its day, as ase-calendar places it (0 to 1)
    std::string tone;         // its domain tone, the <TONE> of DOMAIN_<TONE>
};

/** What the year ring shows this frame. */
struct YearRingFrame {
    uint32_t lun = 0;                 // the current LUN, from 1 (CAL_MONTH)
    uint32_t months_per_year = 0;      // LUN of a VEC (CAL_CONST_MONTHS_PER_YEAR)

    std::vector<uint32_t> season_starts;  // the first LUN of each season, in the order of texts.hud.seasons.*
    std::vector<double> season_light;     // the hours of light of each season, in the same order
    double year_share = 0.0;              // today's share of the year, as ase-calendar places it (0 to 1)

    std::vector<YearRingEvent> events; // the past events

    bool next_due = false;             // the plant lives and has a stage after this one
    uint32_t next_stage = 0;           // the next stage, the index of texts.hud.stages.*
    double next_share = 0.0;           // the share of the year of the day it is reached (0 to 1)

    uint32_t level = 0;                // the perception level (hud.perception.levels)
    double elapsed = 0.0;              // seconds since the sheet opened
    bool still = false;                // every motion held
};

/** The word and the colour of one season on the ring. */
struct YearRingSeason {
    std::vector<HudRun> word;  // texts.hud.seasons.<n>
    ::godot::Color color;      // SEASON_*
};

/** The year ring. */
class YearRing {
public:
    /** Binds measures, colours, words and levels; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the ring of `now` under `frame`; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const YearRingFrame& now,
              std::string& failed);

private:
    /** The season arcs with their words and light and today's line; a missing record is logged as N0460. */
    bool draw_seasons(UiPen& canvas, const ::godot::Transform2D& frame, const YearRingFrame& now,
                      std::string& failed);

    /** The numbered marks, the mark of the current LUN and its stretch. */
    bool draw_months(UiPen& canvas, const ::godot::Transform2D& frame, const YearRingFrame& now,
                     std::string& failed);

    /** The dots of the past events; a missing record is logged as N0450. */
    bool draw_trail(UiPen& canvas, const ::godot::Transform2D& frame, const YearRingFrame& now,
                    std::string& failed);

    /** The next stage; a missing record is logged as N0454. */
    bool draw_next(UiPen& canvas, const ::godot::Transform2D& frame, const YearRingFrame& now,
                   std::string& failed);

    /** The angle of LUN `lun`'s start on a ring of `months` ticks. */
    double month_angle(double lun, uint32_t months) const;

    /** The LUN of mark `mark` of `marks` on a year of `months` LUN: the first LUN that begins at or after it. */
    uint32_t mark_lun(uint32_t mark, uint32_t marks, uint32_t months) const;

    double x_ = 0.0;                    // hud_year_x
    double y_ = 0.0;                    // hud_year_y
    double radius_ = 0.0;               // hud_year_radius
    double season_gap_ = 0.0;           // hud_year_season_gap
    double season_trim_ = 0.0;          // hud_year_season_trim
    double season_width_ = 0.0;         // hud_year_season_width
    double season_stagger_ = 0.0;       // hud_year_season_stagger
    double season_label_gap_ = 0.0;     // hud_year_season_label_gap
    double season_label_size_ = 0.0;    // hud_year_season_label_size
    double season_label_delay_ = 0.0;   // hud_year_season_label_delay
    double light_dy_ = 0.0;             // hud_year_light_dy
    double light_size_ = 0.0;           // hud_year_light_size
    double light_delay_ = 0.0;          // hud_year_light_delay
    double marks_ = 0.0;                // hud_year_marks
    double tick_inner_ = 0.0;           // hud_year_tick_inner
    double tick_outer_ = 0.0;           // hud_year_tick_outer
    double number_inset_ = 0.0;         // hud_year_number_inset
    double number_dy_ = 0.0;            // hud_year_number_dy
    double number_size_ = 0.0;          // hud_year_number_size
    double lun_inset_ = 0.0;            // hud_year_lun_inset
    double lun_trim_ = 0.0;             // hud_year_lun_trim
    double lun_width_ = 0.0;            // hud_year_lun_width
    double lun_alpha_ = 0.0;            // hud_year_lun_alpha
    double today_inner_ = 0.0;          // hud_year_today_inner
    double today_outer_ = 0.0;          // hud_year_today_outer
    double today_line_ = 0.0;           // hud_year_today_line
    double event_inset_ = 0.0;          // hud_year_event_inset
    double event_radius_ = 0.0;         // hud_year_event_radius
    double next_gap_ = 0.0;             // hud_year_next_gap
    double next_radius_ = 0.0;          // hud_year_next_radius
    double next_line_ = 0.0;            // hud_year_next_line
    double next_label_dx_ = 0.0;        // hud_year_next_label_dx
    double next_label_dy_ = 0.0;        // hud_year_next_label_dy
    double next_label_size_ = 0.0;      // hud_year_next_label_size
    double line_hair_ = 0.0;            // hud_line_hair
    uint32_t level_light_ = 0;          // hud.perception.levels[1]

    ::godot::Color soft_, faint_, time_, life_;

    std::vector<HudRun> light_words_;    // texts.hud.light
    std::vector<YearRingSeason> seasons_;             // texts.hud.seasons.* with SEASON_*, in the order of the year
    std::vector<std::vector<HudRun>> stage_words_;    // texts.hud.stages.*, in the order of the stages

    const UiTheme* theme_ = nullptr;     // the theme the domain tones of the events are read from

    UiRing ring_;              // arcs, circles and discs
    UiTextBlock season_;       // class t
    UiTextBlock light_;        // class d
    UiTextBlock number_;       // class d
    UiTextBlock stage_;        // class t
};

}  // namespace ase::adp::godot
