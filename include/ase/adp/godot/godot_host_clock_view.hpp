#pragma once

/**
 * ASE GODOT ADAPTER - HOST CLOCK VIEW
 *
 * @file        godot_host_clock_view.hpp
 * @brief       The game clock of the embedded host as the last snapshot showed it: time, date, day, phases, seasons
 * @description The copy GodotHostResourceManager keeps of the clock, seek, date, day, phase, season and pace
 *              records of ase-pl-flora, every number ase-time's or ase-calendar's own. One purpose of the
 *              dissolved godot_host_resource_manager.hpp (WRFL_ASE_TIER_WIPE, the header ratchet): only the
 *              units that show or read the game time read it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.04.00024 [seed]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <vector>

namespace ase::adp::godot {

/** The game clock as the last snapshot showed it. */
struct VivariumClockView {
    bool   known = false;     // false until a snapshot carried the clock record
    double elapsed_s = 0.0;   // game time, seconds
    double day = 0.0;         // game day
    double hour = 0.0;        // hour of the game day
    double hours_per_day = 0.0;     // hours of a game day, ase-time's own constant (TIM_CONST_HOURS_PER_DAY)
    double seconds_per_hour = 0.0;  // game seconds of a game hour, ase-time's own constant (TIM_CONST_SECONDS_PER_HOUR)
    bool   shoot_known = false;  // false while the clock record carries no cycle end of ase-phytomer's growth clock
    double shoot_s = 0.0;        // game seconds up to which the growth clock ran its cycles (PHY_CLK_LAST_S)
    double real_laid_s = 0.0;    // real position ase-time last set its clock forward to in one write, 0 = never (TIM_REAL_LAID_S)
    bool   seek_known = false;   // false while the clock record carries no verdict of ase-time on a seek or date request
    double seek_seq = 0.0;       // number of the request ase-time last judged (TIM_SEEK_SEQ)
    double seek_order = 0.0;     // ase-time's verdict on it, GODOT_SEEK_ORDER_* (TIM_SEEK_ORDER)
    double seek_target_day = 0.0;  // whole game day of the target ase-time judged it against (TIM_SEEK_TARGET_DAY), GODOT_SEEK_FORM_NONE when refused
    double seek_target_h = 0.0;    // hour of that day (TIM_SEEK_TARGET_H), GODOT_SEEK_FORM_NONE when refused; absolute = day · hours_per_day + hour
    bool   seek_bounds_known = false;  // false while the seek port carries no bounds record of ase-time (ase-pl-flora: flora.seek.v1)
    double seek_seq_min = 0.0;         // lowest request number, the first the host gives a jump, ase-time's own constant (TIM_CONST_SEEK_SEQ_MIN)
    double seek_seq_max = 0.0;         // largest request number a float slot holds exactly, ase-time's own constant (TIM_CONST_SEEK_SEQ_MAX)
    double seek_order_none = 0.0;      // verdict code: no request judged yet, ase-time's own constant (TIM_CONST_SEEK_ORDER_NONE)
    double seek_order_refused = 0.0;   // verdict code: the request named no target the clock can reach, the largest code (TIM_CONST_SEEK_ORDER_REFUSED)
    bool   date_known = false;  // false while the date port carries no SURA date record (no measured CAL_YEAR yet)
    double period = 0.0;      // code of the day period, GODOT_PERIOD_FIRST..GODOT_PERIOD_LAST (TIM_DAY_PERIOD)
    double year = 0.0;        // SURA VECTUM (CAL_YEAR)
    double epoch = 0.0;       // SURA AETRIX (CAL_EPOCH)
    double cycle = 0.0;       // SURA CYCLUM (CAL_CYCLE)
    double month = 0.0;       // SURA LUNBREX, from 1 (CAL_MONTH)
    double month_day = 0.0;   // SURA TUMBRAE, from 1 (CAL_DAY)
    double year_day = 0.0;    // the day of the VECTUM, from 1, part of the day record (CAL_YEAR_DAY; flora.day.v1, N0460)
    double days_per_month = 0.0;   // TUMBRAE of a LUNBREX, ase-calendar's own constant (CAL_CONST_DAYS_PER_MONTH)
    double months_per_year = 0.0;  // LUNBREX of a VECTUM, ase-calendar's own constant (CAL_CONST_MONTHS_PER_YEAR)
    double season_glacix = 0.0;     // code of GLACIX, winter, as ase-calendar numbers it on the date record (CAL_CONST_SEASON_GLACIX)
    double season_saltatrix = 0.0;  // code of SALTATRIX, spring (CAL_CONST_SEASON_SALTATRIX)
    double season_ignix = 0.0;      // code of IGNIX, summer (CAL_CONST_SEASON_IGNIX)
    double season_halitrix = 0.0;   // code of HALITRIX, autumn (CAL_CONST_SEASON_HALITRIX)
    bool   season_codes_known = false;  // true only when the date record of ase-calendar carried the four codes; the HUD's stand-in date never sets it
    bool   day_known = false;   // N0460: false while the day port carries no day record (ase-pl-flora: flora.day.v1)
    double sunrise_h = 0.0;     // the hour the sun rises this day
    double sunset_h = 0.0;      // the hour the sun sets this day
    double daylight_h = 0.0;    // the hours of daylight this day (CAL_DAYLIGHT_DURATION)
    double season = 0.0;        // the season, CAL_CONST_SEASON_GLACIX .. HALITRIX (CAL_SEASON)
    double day_share = 0.0;     // the clock's share of the day, 0 to 1 (TIM_DAY_PROGRESS)
    double sunrise_share = 0.0; // the share of the day the sun rises at
    double sunset_share = 0.0;  // the share of the day the sun sets at
    double year_share = 0.0;    // today's share of the year, 0 to 1 (ase-calendar)
    double lun_share = 0.0;     // the share of the year a whole LUNBREX fills (CAL_LUN_SHARE): the year ring places LUN m at (m - 1) times it (B0280)
    bool   phases_known = false;  // N0460: false while the phase port carries no full set of phase records (flora.phase.v1)
    std::vector<double> phase_start_h;      // the hour each phase begins, in the order of GODOT_PERIOD_*
    std::vector<double> phase_start_share;  // the share of the day each phase begins at, in the same order
    bool   seasons_known = false;  // N0460: false while the season port carries no full set of season records (flora.season.v1)
    std::vector<double> season_start_lun;   // the first LUN of each season, in the order of CAL_CONST_SEASON_*
    std::vector<double> season_light_h;     // the hours of light of each season, in the same order
    bool   pace_known = false;  // N0459: false while the pace port carries no pace record (ase-pl-flora: flora.pace.v1)
    double pace_factor = 0.0;   // the multiple of ase-time's base pace the clock runs at (FloraStaPaceComponent::factor)
};

}  // namespace ase::adp::godot
