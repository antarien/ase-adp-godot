/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW DATE REWIND
 *
 * @file        godot_vivarium_rewind.cpp
 * @brief       VivariumView - the jump to a date ase-time judged behind its clock
 * @description ase-time's clock only runs forward: TimeClkSeekCalcSystem judges a target behind it BEHIND and
 *              leaves the clock where it stands, because every module that takes a step from the elapsed game
 *              time would get a negative one. A date jump ase-time judged BEHIND (godot_vivarium_jump.cpp,
 *              VivariumView::follow_debug) therefore goes back the way a start of the app does - with the loading
 *              screen of a restart, as the operator order asks. The view compares no date: the direction is
 *              ase-time's verdict on the request number of the jump.
 *
 *              THE REWIND (rewind_host) is the teardown of Reiniciar followed by a new host with the seeds of this
 *              run - the same birch from its start, never a new sowing - and the pace that stood before, sent
 *              again through the pace port. THE DATE AGAIN (follow_rewind) follows once the new host's clock
 *              record stands: from the start clock of the run it goes through the date port with the next request
 *              number, in one write (VivariumView::send_date); ase-calendar turns the date into the game hour with
 *              its own constants and ase-time judges it against its start clock. AHEAD or AT lands the jump like
 *              every date ahead; BEHIND now means the date lies before the start clock of the run and ends the
 *              jump with that reason. Everything up to the date is computed by the modules at the new time; the
 *              view computes no calendar and the calendar is never written.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066, DSGN_046
 * @category    time/period/cycle
 * @usecase     usecase:101120 - clock the growth cycle
 * @usecase     usecase:100390 - return to the garden
 * @actor       Growth Clock Controller (GROWCLOCON), Plant Keeper (PLNTKEEPER)
 * @created     2026-11-09
 * @modified    2026-11-10
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/kernel/kernel_types.hpp>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

void VivariumView::rewind_host(const VivariumJump& jump) {
    // THE REWIND: the same teardown as Reiniciar, a new host with this run's seeds, the pace that stood. The
    // start screen stands from the boot on; the date is owed again until the new clock record stands. The
    // request numbers of the run go on growing (VivariumDebug::seq), so no verdict of the old host answers it.
    // `jump` is debug_.jump itself (follow_debug), and boot_host resets debug_.jump: the jump is kept first.
    const VivariumJump kept = jump;
    const double factor = debug_.factor;
    note("debug date rewind sowing=" + ::godot::String::num_uint64(run_sowing_) +
         " factor=" + ::godot::String::num(factor, GODOT_LOG_DECIMALS) + " " + measure(GODOT_NO_PLACE));
    resources_.clear_all();
    debug_.factor = GODOT_PACE_NONE;
    drain_log();
    boot_host(false);
    if (!failure_text_.is_empty()) {
        node_->queue_redraw();
        return;
    }
    if (factor != GODOT_PACE_NONE) {
        const HostStatus status = resources_.pace(factor);
        note("debug date rewind pace factor=" + ::godot::String::num(factor, GODOT_LOG_DECIMALS) +
             " result=" + GodotHostResourceManager::status_name(status));
        if (status == ase::kernel::HostStatusOk) {
            debug_.factor = factor;
        }
    }
    VivariumJump next = kept;
    next.ahead = false;
    next.rewound = true;
    next.sought_run_s = GODOT_HOUR_NONE;
    debug_.jump = next;
    raise_jump_screen();
}

void VivariumView::follow_rewind(const VivariumClockView& clock) {
    // THE DATE AGAIN FROM THE START CLOCK: the new clock record stands at the start clock of the run, and the
    // date goes through the date port once more with the next request number. ase-time's verdict on it ends the
    // jump in follow_debug: AHEAD or AT lands it, BEHIND names a date before the run.
    VivariumJump& jump = debug_.jump;
    jump.rewound = false;
    const HostStatus status = send_date(clock);
    note("debug date rewind seek year=" + ::godot::String::num(jump.target_year, 0) +
         " month=" + ::godot::String::num(jump.target_month, 0) +
         " month_day=" + ::godot::String::num(jump.target_month_day, 0) +
         " hour=" + ::godot::String::num(jump.target_hour, 0) +
         " seq=" + ::godot::String::num(jump.seq, 0) +
         " from_hour=" + ::godot::String::num(clock.hour, GODOT_LOG_DECIMALS) +
         " result=" + GodotHostResourceManager::status_name(status));
    if (status != ase::kernel::HostStatusOk) {
        jump = VivariumJump{};
        node_->queue_redraw();
    }
}

}  // namespace ase::adp::godot
