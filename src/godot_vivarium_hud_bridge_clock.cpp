/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD CLOCK BRIDGE
 *
 * @file        godot_vivarium_hud_bridge_clock.cpp
 * @brief       VivariumView - the clock record and the factor it runs at while their ports carry no record yet
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5, design decision 2026-11-10: the clock of the care view (the
 *              hour, the SURA date, the day, its phases and the seasons) and the hourglass's factor draw from their
 *              frame and never ask whether a port delivered. Where a port leaves a group empty, the bridge writes the
 *              value of the accepted mockup (hud_bridge_clock_* and hud_bridge_pace_factor of SHA, read by
 *              VivariumView::load_hud_bridge) and logs the gap once on its edge (open_gap) with the port field it
 *              reads first; the moment the port's record stands, the closing edge is logged (close_gap) and the
 *              port's values are drawn. Nothing of this stands in the picture.
 *
 *              BRIDGE GAPS (register carrier N0448): N0460 the clock groups - the hour of ase-time, the SURA date of
 *              ase-calendar, the day, phase and season records of ase-pl-flora (flora.day.v1, flora.phase.v1,
 *              flora.season.v1). N0459 the factor the clock runs at (flora.pace.v1 factor): the last asked, else
 *              hud_bridge_pace_factor.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0460 (clock groups), N0459 (flora.pace.v1 factor), carrier N0448
 * @created     2026-11-14
 * @modified    2026-11-14
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/types.hpp>

#include <vector>

namespace ase::adp::godot {

void VivariumView::bridge_pace(HourglassFrame& glass) {
    // N0459: flora.pace.v1 reports the factor the clock runs at (FloraHostPaceExptSystem), and the hourglass shows
    // that one. Before the port filed a record the gap stays open: the hourglass then shows the factor last asked
    // through the debug layer (VivariumView::ask_pace) and, before any was asked, the mockup's S.factor
    // (hud_bridge_pace_factor of SHA).
    if (hud_clock_shown_.pace_known) {
        close_gap(GODOT_HUD_GAP_PACE, "flora.pace.v1.factor");
        glass.factor = hud_clock_shown_.pace_factor;
        return;
    }
    open_gap(GODOT_HUD_GAP_PACE, "flora.pace.v1.factor");
    glass.factor = debug_.factor > GODOT_PACE_NONE ? debug_.factor : hud_bridge_pace_factor_;
}

void VivariumView::bridge_clock(VivariumClockView& clock) {
    // THE EDGE OF THE BRIDGE FOR N0460: each group of the clock record the ports leave empty is the mockup's
    // (hud_bridge_clock_* of SHA), its gap logged once with the port field named, and closed on the edge the port's
    // group stands. The port's own fields stand first: a measured hour or date keeps its value, and every share is
    // computed from what stands, by the mockup's rules (hA: an hour over the hours of the day; day: the day of the
    // year less one over the days of the year).
    const VivariumClockView& mock = hud_bridge_clock_;
    if (!clock.known) {
        open_gap(GODOT_HUD_GAP_CLOCK, "time.hour");  // ase-time: TIM_DAY_HOUR, TIM_CONST_HOURS_PER_DAY, TIM_DAY_PERIOD
        clock.hour = mock.hour;
        clock.hours_per_day = mock.hours_per_day;
        clock.period = mock.period;
        clock.known = true;
    } else {
        close_gap(GODOT_HUD_GAP_CLOCK, "time.hour");
    }
    if (!clock.date_known) {
        // ase-calendar: CAL_YEAR, CAL_MONTH, CAL_DAY, CAL_CONST_DAYS_PER_MONTH, CAL_CONST_MONTHS_PER_YEAR.
        open_gap(GODOT_HUD_GAP_CLOCK, "calendar.date");
        clock.year = mock.year;
        clock.month = mock.month;
        clock.month_day = mock.month_day;
        clock.days_per_month = mock.days_per_month;
        clock.months_per_year = mock.months_per_year;
        clock.date_known = true;
    } else {
        close_gap(GODOT_HUD_GAP_CLOCK, "calendar.date");
    }
    const double hours = clock.hours_per_day > 0.0 ? clock.hours_per_day : mock.hours_per_day;
    if (!clock.day_known) {
        // ase-pl-flora flora.day.v1: sunrise_h, sunset_h, daylight_h, season, day_share, year_share and year_day,
        // the day of the VECTUM from 1 (CAL_YEAR_DAY).
        open_gap(GODOT_HUD_GAP_CLOCK, "flora.day.v1");
        clock.year_day = mock.year_day;
        clock.sunrise_h = mock.sunrise_h;
        clock.sunset_h = mock.sunset_h;
        clock.daylight_h = mock.daylight_h;
        clock.season = mock.season;
        clock.day_share = clock.hour / hours;
        clock.sunrise_share = clock.sunrise_h / hours;
        clock.sunset_share = clock.sunset_h / hours;
        const double per_month = clock.days_per_month > 0.0 ? clock.days_per_month : mock.days_per_month;
        const double per_year = clock.months_per_year > 0.0 ? clock.months_per_year : mock.months_per_year;
        const double day_of_year = (clock.month - 1.0) * per_month + clock.month_day;
        clock.year_share = (day_of_year - 1.0) / (per_month * per_year);
        clock.day_known = true;
    } else {
        close_gap(GODOT_HUD_GAP_CLOCK, "flora.day.v1");
    }
    if (!clock.phases_known) {
        // ase-pl-flora flora.phase.v1: start_h and start_share of each period, in the order of GODOT_PERIOD_*.
        open_gap(GODOT_HUD_GAP_CLOCK, "flora.phase.v1");
        clock.phase_start_h = mock.phase_start_h;
        clock.phase_start_share.clear();
        for (const double start_h : clock.phase_start_h) {
            clock.phase_start_share.push_back(start_h / hours);
        }
        clock.phases_known = true;
    } else {
        close_gap(GODOT_HUD_GAP_CLOCK, "flora.phase.v1");
    }
    if (!clock.seasons_known) {
        // ase-pl-flora flora.season.v1: start_lun and light_h of each season, in the order of CAL_CONST_SEASON_*.
        open_gap(GODOT_HUD_GAP_CLOCK, "flora.season.v1");
        clock.season_start_lun = mock.season_start_lun;
        clock.season_light_h = mock.season_light_h;
        clock.seasons_known = true;
    } else {
        close_gap(GODOT_HUD_GAP_CLOCK, "flora.season.v1");
    }
}

}  // namespace ase::adp::godot
