#pragma once

/**
 * ASE GODOT ADAPTER - HUD DEBUG RSM TABLE
 *
 * @file        godot_hud_debug_rsm.hpp
 * @brief       What the RSM table of the debug layer shows: its entries, the clock, the column order and the held set
 * @description MIG_ASE_SKILL_RSM step B4, usecase:101305 show skill progress. The adapter shows the last set of
 *              avatar.skill.v1 in every frame and counts nothing itself; this unit turns that set into the numbers
 *              the table prints.
 *
 *              THE CLOCK is CountdownTimerFormater.format of the prototype: whole seconds as HH:MM:SS, each part of
 *              two digits, the hours growing past two digits where they must. A value that is no number prints "-",
 *              as the prototype's empty timer did.
 *              THE COLUMNS are one per learning entry, the newest left. An entry entity is made after the ones
 *              before it, so the newest is the one of the highest id.
 *              QNT is the countdown of the running time slice: the server sends the seconds until it ends (qnt_left,
 *              avatar.skill.qnt.v1) and the table prints that cell as it is.
 *              THE TABLE STANDS ALWAYS, as the timer table of the prototype (simple.xml) did: where the port names
 *              no entry, the set shown last stays with its last values, so the last active entry is still read
 *              after it ended; before the first learning order no set was shown, and the table shows one column of
 *              zeros under the idle head (texts.hud.debug_rsm.idle).
 *              THE SHARES read the whole set: DCD is the largest share of the grace left and QNT the largest share of
 *              the running slice, both the server's own shares (dcd_shr, qnt_shr); ACD is the largest acd of acd_max
 *              and TUC the largest tuc of tuc_max, each 0..1; an entry that carries no length of its own takes no part.
 *              The tab RSM draws no ring of them: it carries the timer table alone (operator 2026-11-25).
 *              THE SLICES OF AN ENTRY have no order on the wire, only their ring place (slot) and the place of the
 *              running slice (slc_head) of the ring of slc_len places booked so far: back = (slc_head - slot + slc_len)
 *              % slc_len, the newest at back 0. The slices carry no age: the server sends none.
 *              THE VIEW: the chip a tap hit in the title row of the open debug drawer chooses one of its three tab
 *              layers (SYSTEM, RSM, STATUS); a hit that names no view leaves the choice as it stands.
 *
 *              NO GODOT TYPE, so the host tests drive it with any set of entries. DebugRsmTable
 *              (godot_hud_debug_rsm_draw.hpp) draws it.
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

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One learning entry as the table reads it: the values of one record of avatar.skill.v1; a missing key is NaN. */
struct DebugRsmEntry {
    uint32_t id = 0;            // the entry entity: the later the entry, the higher the id
    double action = 0.0;        // BDI_CONST_ACTION_*, the entry's skill_id: the head of the column
    double phase = 0.0;         // 0 active, 1 after-run, 2 relearn, 3 locked, 4 dormant
    double ses = 0.0;           // session seconds
    double acd = 0.0;           // fluctuation_current
    double acd_max = 0.0;       // fluctuation_max
    double tuc = 0.0;           // seconds
    double tuc_max = 0.0;       // seconds: the length the ring TUC is the share of
    double dcd = 0.0;           // remaining grace seconds
    double dcd_max = 0.0;       // grace length, seconds: rsm_grace_length(dcd, dcd_shr) of the port values
    double dcd_shr = 0.0;       // the share of the grace left, 0 to 1, as the server computed it
    double qnt = 0.0;           // seconds the running time slice has run
    double qnt_max = 0.0;       // length of the running time slice, seconds: rsm_slice_length(qnt, qnt_left)
    double qnt_left = 0.0;      // seconds until the running time slice ends: the QNT cell
    double qnt_shr = 0.0;       // the share of the running time slice that has run, 0 to 1
    double tul = 0.0;           // seconds
    double que_fill = 0.0;      // 0 to 1
    double lck = 0.0;           // lock seconds
    double dgexp = 0.0;         // dgEXP
    double prg = 0.0;           // progress toward the next plateau, 0 to 100: the PRG cell (avatar.skill.prg.v1)
    double vel = 0.0;           // smoothed rate of experience: the VEL cell
    double msr = 0.0;           // seconds the entry has been measured: the MSR cell
    double vol = 0.0;           // units of work booked while the session runs: the VOL cell
};

/** The rings: the largest share of the grace left, of the fluctuation, of the running slice and of the TUC, each 0 to 1. */
struct DebugRsmShares {
    double dcd = 0.0;           // max over the entries of dcd_shr, the server's share of the grace left
    double acd = 0.0;           // max over the entries of acd / acd_max
    double qnt = 0.0;           // max over the entries of qnt_shr, the server's share of the running slice
    double tuc = 0.0;           // max over the entries of tuc / tuc_max
};

/** `seconds` as HH:MM:SS (whole seconds, a value under 0 as 0); "-" where it is no number. */
std::string debug_rsm_clock(double seconds);

/** `value` as a whole number (cut, not rounded); "-" where it is no number or too large to count. */
std::string debug_rsm_whole(double value);

/** The countdown of the running slice, qnt_max - qnt, never under 0; NaN where either is no number. */
double debug_rsm_left(double qnt, double qnt_max);

/** At most `limit` entries of `in`, the newest (highest id) first, in `out`. */
void debug_rsm_columns(const std::vector<DebugRsmEntry>& in, uint32_t limit, std::vector<DebugRsmEntry>& out);

/** The shares the two rings show over all of `in`; 0 and 0 where no entry carries a length. */
DebugRsmShares debug_rsm_shares(const std::vector<DebugRsmEntry>& in);

/** The columns the table shows in `out`: the newest `limit` of `in` (debug_rsm_columns), which `held` keeps for the
 *  next frame; where `in` is empty the set `held` kept, so the last active entry stays until a new one comes. `out` is
 *  empty only before any set was shown: then the table draws its column of zeros. */
void debug_rsm_shown(const std::vector<DebugRsmEntry>& in, uint32_t limit, std::vector<DebugRsmEntry>& held,
                     std::vector<DebugRsmEntry>& out);

/** The length of the running slice, seconds: the seconds it has run plus the seconds until it ends; NaN where either is no number. */
double rsm_slice_length(double qnt, double qnt_left);

/** The grace length, seconds: the grace left divided by its share; NaN where the share is no number or not above 0. */
double rsm_grace_length(double dcd, double dcd_shr);

/** Places a slice stands behind the running one: (head - slot + places) % places; NaN where the ring is no ring. */
double rsm_slice_back(double head, double slot, double places);

/** The seconds a slice has stood: `qnt` of its entry + `back` * `qnt_max`; NaN where any is no number. */
double rsm_slice_age(double qnt, double back, double qnt_max);

/** The view after a tap: `hit` where it is a view of GODOT_HUD_DEBUG_VIEWS, else `current`. */
uint32_t debug_view_choose(uint32_t current, uint32_t hit);

/** The place of action code `action` (an entry's skill_id, BDI_CONST_ACTION_*) among the column words, 0 for
 *  GODOT_RSM_ACTION_FIRST; false where it is no number or lies outside GODOT_RSM_ACTION_FIRST to _LAST. */
bool debug_rsm_action_slot(double action, uint32_t& out);

}  // namespace ase::adp::godot
