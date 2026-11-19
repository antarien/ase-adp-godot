/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW JUMP
 *
 * @file        godot_vivarium_jump.cpp
 * @brief       VivariumView - the jump of the debug mode: to a stage, an hour, a SURA date, an eclipse, a moonrise
 * @description THE JUMP (debug.jumps, read and drawn by godot_vivarium_debug.cpp) sets the clock to a target: a
 *              stage of the selected patch, an hour of the game day, a SURA date whole VECTUM ahead, the maximum
 *              of the next eclipse of the sun or the moon, or the next rise of a moon. All of them are the same
 *              case, only the recognition of the target differs (operator order of 2026-11-06).
 *
 *              A JUMP IS ONE WRITE (operator order of 2026-11-09): the target goes once into ase-time's clock, in
 *              the form its source names it - through the seek port (VivariumView::send_seek, FloraHostSeekSystem
 *              files TIM_REQ_TIME_SEEK_*) as an absolute game hour, as hours ahead of the clock or as an hour of
 *              the game day, or through the date port as a SURA date (VivariumView::send_date,
 *              CalendarDatSeekCalcSystem turns it into the game hour with its own constants). ase-time lays the
 *              target from its own clock, judges it and sets the clock there in one write when it lies ahead
 *              (TimeClkSeekCalcSystem). From there every module reads the new time from the star in its own
 *              schedule and computes its state at it, the same computation as a start of the app at that time:
 *              the calendar and the sky in closed form, the stands over the span since their last step, the
 *              shoot over every day in between in one cycle (PhytomerClkCalcSystem). The view computes no game
 *              hour, compares no clock, feeds no seconds beyond the frame, steps no waypoint and cares for no
 *              patch while a jump runs. The pace stays as it was before the jump; the jump never touches the pace
 *              port.
 *
 *              THE TARGET comes from the star, never from the view:
 *              an HOUR is the button's hour of the game day, sent as such; ase-time lays its next occurrence from
 *              its own clock;
 *              a STAGE is the forecast of ase-plant (PlantStgeFcstSystem, PLT_GROW_AT_H for Crescendo,
 *              PLT_MTUR_AT_H for Madura, absolute game hours); Broto comes from the keeper's pour lifting the
 *              soil to PLT_SPEC_GERM_REL and carries no forecast, so its jump is refused as unforecast;
 *              an ECLIPSE is the prediction of ase-ephemeris (EphemerisEclNextSystem, next_solar and next_lunar
 *              of the sky port, game hours from now), sent as hours ahead;
 *              a MOONRISE is the forecast of ase-ephemeris (next_moonrise of the sky port, game hours from now;
 *              godot_vivarium_moonrise.cpp), sent as hours ahead;
 *              a DATE is the running VECTUM plus the button's years, or any date of the free entry
 *              (godot_vivarium_date.cpp).
 *
 *              THE END (follow_debug) is ase-time's verdict on the jump's request number in the clock record
 *              (VivariumClockView::seek_seq, seek_order, seek_target_day, seek_target_h): REFUSED ends at once; BEHIND rebuilds the
 *              host for a date with this run's seeds and pace and sends the date again from its start clock
 *              (godot_vivarium_rewind.cpp), and ends every other jump; AHEAD and AT end the jump on the verdict:
 *              running falls there, and the start screen lifts on the same criteria as on a start of the app
 *              (VivariumView::follow_initialisation). The landing note waits until the host simulated
 *              debug.settle_s at its own pace since the write (VivariumJump::sought_run_s, cleared by seek_jump
 *              and send_date, stamped by follow_debug at ase-time's first verdict on the request), so the
 *              schedules of the slower tiers ran their passes at the new time before it is written. The settle
 *              feeds no second, stops no system and holds no picture: every module runs on at the pace of the
 *              run, only the note waits. On landing the note says what the
 *              star shows there: the stage of the watched patch, the standing eclipse record, the moons in the
 *              camera's frustum. A landing that misses its sight (a stage not reached, no eclipse standing at its
 *              predicted maximum) is noted as the producing module's finding. Only the eclipse jump searches on
 *              (D2256): ase-ephemeris names the end of its horizon for a kind without an eclipse in reach, and a
 *              reader that jumps by it searches on from there (CAUSA_ASE_EPHEMERIS, EphemerisEclNextSystem), so
 *              the jump takes the prediction made at the new time, up to debug.eclipse_hops times.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066, DSGN_046
 * @category    time/period/cycle
 * @usecase     usecase:101120 - clock the growth cycle
 * @usecase     usecase:100390 - return to the garden
 * @actor       Growth Clock Controller (GROWCLOCON), Plant Keeper (PLNTKEEPER)
 * @created     2026-11-07
 * @modified    2026-11-19
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/kernel/kernel_types.hpp>

#include <cstdint>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

void VivariumView::raise_jump_screen() {
    // THE JUMP BEGINS LIKE A START OF THE APP AT THE NEW TIME: the start screen stands again, the scene stops
    // following the ports, and the rate measurement starts anew from the lift - two readings across the jump
    // would measure the jump, never the clock. ase-time's constants stay; the step of the date entry needs them.
    initialised_ = false;
    init_render_frames_ = 0u;
    hour_length_.last_elapsed_s = -1.0;
    hour_length_.last_day = -1.0;
    hour_length_.last_hour = -1.0;
    hour_length_.last_simulated_s = -1.0;
    node_->queue_redraw();
}

bool VivariumView::seek_jump(VivariumJump& next, double game_hour, double ahead_h, double day_hour,
                             const ::godot::String& words) {
    // THE ONE WRITE of every jump through the seek port: the target in the form its source names, with the
    // next request number of the run. A refused write ends the jump before it began; a written one raises the
    // start screen until ase-time's verdict on it ends it.
    const HostStatus status = send_seek(next, game_hour, ahead_h, day_hour);
    note("debug jump " + words + " seq=" + ::godot::String::num(next.seq, 0) +
         " result=" + GodotHostResourceManager::status_name(status));
    if (status != ase::kernel::HostStatusOk) {
        return false;
    }
    next.sought_run_s = GODOT_HOUR_NONE;  // stamped at the clock's write, ase-time's first verdict on next.seq
    debug_.jump = next;
    raise_jump_screen();
    return true;
}

void VivariumView::ask_jump(const VivariumDebugJump& jump, uint32_t index) {
    // THE JUMP never touches the pace port: the clock is set to the target through the seek or the date port,
    // every module computes its state at the new time, and the pace asked before stays.
    const VivariumClockView& clock = resources_.get_clock();
    VivariumJump next;
    next.running = true;
    next.button = index;
    next.asked_day = clock.day;
    if (jump.years > GODOT_YEARS_NONE) {
        // A DATE JUMP moves the clock to a SURA date: the running VECTUM plus the button's years, with month,
        // day and hour of the clock kept. ase-calendar turns the date into the game hour with its own constants
        // and ase-time sets its clock there; the calendar is never written.
        const ::godot::String years_word = ::godot::String::num(jump.years, 0);
        if (!clock.known || !clock.date_known) {
            note("debug jump years=" + years_word + " result=unmeasured");
            return;
        }
        next.target_year = clock.year + jump.years;
        next.target_month = clock.month;
        next.target_month_day = clock.month_day;
        next.target_hour = clock.hour;
        next.ahead = true;
        debug_.jump = next;
        const HostStatus status = send_date(clock);
        note("debug jump years=" + years_word + " from_year=" + ::godot::String::num(clock.year, 0) +
             " target_year=" + ::godot::String::num(next.target_year, 0) +
             " result=" + GodotHostResourceManager::status_name(status));
        if (status != ase::kernel::HostStatusOk) {
            debug_.jump = VivariumJump{};
            return;
        }
        raise_jump_screen();
        return;
    }
    if (!clock.known) {
        note("debug jump button=" + ::godot::String::num_uint64(index) + " result=unmeasured");
        return;
    }
    if (jump.stage != GODOT_STAGE_NONE) {
        // A STAGE JUMP watches the selected patch and lands on ase-plant's forecast of the stage, an absolute
        // game hour. Whether it lies ahead of the clock is ase-time's verdict, never the view's.
        const ::godot::String stage_word = "stage=" + ::godot::String::num_uint64(jump.stage);
        const VivariumPatchView* patch =
            selected_place_ == GODOT_NO_PLACE ? nullptr : resources_.get_patch_at(selected_place_);
        if (patch == nullptr) {
            note("debug jump " + stage_word + " result=disabled");
            return;
        }
        if (patch->stage != GODOT_CODE_UNKNOWN && patch->stage >= jump.stage) {
            note("debug jump " + stage_word + " result=reached " + measure(selected_place_));
            return;
        }
        // The forecast of the target position, by the key of the stage record it is read under (usecase:101635).
        const double due_h = resources_.stage_due_h(*patch, jump.stage);
        if (!(due_h >= 0.0)) {
            // No forecast: Broto carries none, an unmeasured forecast is NaN.
            note("debug jump " + stage_word + " due_h=" + ::godot::String::num(due_h, GODOT_LOG_DECIMALS) +
                 " result=unforecast " + measure(selected_place_));
            return;
        }
        next.stage = jump.stage;
        next.place = selected_place_;
        (void)seek_jump(next, due_h, GODOT_SEEK_FORM_NONE, GODOT_SEEK_FORM_NONE,
                        stage_word + ::godot::String(" due_h=") + ::godot::String::num(due_h, GODOT_LOG_DECIMALS) + " " +
                            measure(selected_place_));
        return;
    }
    if (jump.eclipse_low != GODOT_ECLIPSE_NONE) {
        // AN ECLIPSE JUMP lands on ase-ephemeris' prediction of the next eclipse of the button's kinds, sent as
        // hours ahead of the clock. The view computes no orbit; the times are ase-ephemeris' and ase-celestial's.
        const ::godot::String kinds_word = "eclipse=" + ::godot::String::num_uint64(jump.eclipse_low) + ".." +
                                           ::godot::String::num_uint64(jump.eclipse_high);
        const VivariumSkyView& sky = resources_.get_sky();
        const bool solar = jump.eclipse_high <= GODOT_ECLIPSE_SOLAR_MAX;
        double predicted_h = GODOT_HOUR_NONE;
        if (sky.known && sky.eclipse_nexts > 0u) {
            predicted_h = solar ? sky.eclipse_next[0].next_solar : sky.eclipse_next[0].next_lunar;
        }
        if (!(predicted_h > 0.0)) {
            note("debug jump " + kinds_word + " predicted_h=" + ::godot::String::num(predicted_h, GODOT_LOG_DECIMALS) +
                 " result=unforecast");
            return;
        }
        next.eclipse_low = jump.eclipse_low;
        next.eclipse_high = jump.eclipse_high;
        (void)seek_jump(next, GODOT_SEEK_FORM_NONE, predicted_h, GODOT_SEEK_FORM_NONE,
                        kinds_word + ::godot::String(" predicted_h=") +
                            ::godot::String::num(predicted_h, GODOT_LOG_DECIMALS));
        return;
    }
    if (jump.moonrise) {
        ask_moonrise(next);  // the next rise of a moon (godot_vivarium_moonrise.cpp)
        return;
    }
    // AN HOUR JUMP: the button's hour of the game day, sent as such. ase-time lays its next occurrence from its
    // own clock - this day while it lies ahead, the next once the clock passed it.
    next.hour = jump.hour;
    (void)seek_jump(next, GODOT_SEEK_FORM_NONE, GODOT_SEEK_FORM_NONE, jump.hour,
                    "hour=" + ::godot::String::num(jump.hour, GODOT_LOG_DECIMALS) +
                        " day=" + ::godot::String::num(clock.day, 0) +
                        " clock_hour=" + ::godot::String::num(clock.hour, GODOT_LOG_DECIMALS));
}

void VivariumView::follow_debug() {
    // THE END: a jump ends on ase-time's verdict on its own request number. Nothing is sent while it runs - the
    // one write went out with the ask, and every module computes its state at the new time in its own schedule.
    // After an AHEAD or AT verdict the jump no longer runs, while its landing note is still owed: the stamp of
    // the write (sought_run_s) stands until the settle ran and the note is written.
    VivariumJump& jump = debug_.jump;
    if (!jump.running && jump.sought_run_s < 0.0) {
        return;
    }
    const VivariumClockView& clock = resources_.get_clock();
    if (!clock.known) {
        return;  // no record yet in this frame; the next snapshot carries it
    }
    if (jump.rewound) {
        // A rewound host writes its date once its own clock record stands (godot_vivarium_rewind.cpp).
        if (clock.date_known) {
            follow_rewind(clock);
        }
        return;
    }
    if (!clock.seek_known || clock.seek_seq != jump.seq) {
        return;  // ase-time has not judged this request yet; its verdict follows in a later snapshot
    }
    const int order = static_cast<int>(clock.seek_order);
    // ase-time judges the target as whole game day and hour of that day (TIM_SEEK_TARGET_DAY, TIM_SEEK_TARGET_H),
    // since an absolute game hour outgrows a float slot; the line hands both on as they stand.
    const ::godot::String verdict_words = " seq=" + ::godot::String::num(jump.seq, 0) +
                                          " order=" + ::godot::String::num_int64(order) +
                                          " target_day=" + ::godot::String::num(clock.seek_target_day, 0) +
                                          " target_h=" + ::godot::String::num(clock.seek_target_h, GODOT_LOG_DECIMALS);
    const bool date = jump.target_year >= 0.0;
    if (clock.seek_bounds_known && order == static_cast<int>(clock.seek_order_refused)) {
        note("debug jump ended reason=refused" + verdict_words + " " + measure(selected_place_));
        jump = VivariumJump{};
        node_->queue_redraw();
        return;
    }
    if (order == GODOT_SEEK_ORDER_BEHIND) {
        if (date && jump.ahead) {
            // A DATE BEHIND THE CLOCK: ase-time's clock only runs forward, so the host is built anew with this
            // run's seeds and pace and the date goes out again from its start clock.
            note(::godot::String("debug jump ") + (jump.entry ? "entry" : "years") + " direction=back" +
                 verdict_words + " result=rewind");
            rewind_host(jump);
            return;
        }
        // Behind the start clock of a rewound host, or a game hour behind the clock: the clock cannot go there.
        note(::godot::String("debug jump ended reason=") + (date ? "before_start" : "behind") + verdict_words +
             " " + measure(selected_place_));
        jump = VivariumJump{};
        node_->queue_redraw();
        return;
    }
    // AHEAD or AT: the clock stands at the target, and THE JUMP ENDS HERE (D2281): running falls on the verdict,
    // and the start screen lifts on the same criteria as on a start of the app (follow_initialisation) - the
    // scene built, the sky and the clock known, GODOT_INIT_RENDER_FRAMES quiet frames. THE SETTLE AT THE NEW
    // TIME holds only the landing note, never the view: the slower tiers (Cyclic at 1 Hz and below) run their
    // first pass at the new time only after their interval of simulated seconds, so the note waits until
    // debug.settle_s ran at the pace of the run since the write. THE WRITE is the first snapshot whose verdict
    // answers this request number: there the settle is stamped, so the run from the request to ase-time's
    // write never spends it.
    // THE SETTLE COUNTS IN STAGE TIME (B0276): the seconds the App ticked, each clamped to the scheduler's
    // frame, since the slow tiers run after their interval of exactly these seconds. The real time of the
    // frames runs ahead of it whenever a frame outlasts the frame ceiling, and would spend the settle before
    // the last tier of the chain had its pass at the new time.
    if (jump.sought_run_s < 0.0) {
        jump.sought_run_s = resources_.stage_seconds();
        jump.running = false;
        note("debug jump written" + verdict_words + " run_s=" +
             ::godot::String::num(jump.sought_run_s, GODOT_LOG_DECIMALS));
        node_->queue_redraw();
        return;
    }
    const bool settled = jump.sought_run_s >= 0.0 &&
                         resources_.stage_seconds() - jump.sought_run_s >= debug_.settle_s;
    if (!settled) {
        return;
    }
    const ::godot::String at_words = verdict_words + ::godot::String(" elapsed_s=") + ::godot::String::num(clock.elapsed_s, GODOT_LOG_DECIMALS) +
                                     " year=" + ::godot::String::num(clock.year, 0) +
                                     " month=" + ::godot::String::num(clock.month, 0) +
                                     " month_day=" + ::godot::String::num(clock.month_day, 0) +
                                     " hour=" + ::godot::String::num(clock.hour, GODOT_LOG_DECIMALS) +
                                     " day=" + ::godot::String::num(clock.day, 0);
    if (date) {
        // THE LANDING OF A DATE JUMP: ase-calendar shows the date computed from the new time.
        note(::godot::String("debug jump reached ") + (jump.entry ? "entry" : "years") +
             " direction=" + (jump.ahead ? "ahead" : "back") + at_words + " " + measure(selected_place_));
    } else if (jump.stage != GODOT_STAGE_NONE) {
        // THE LANDING OF A STAGE JUMP: the stage the patch shows at ase-plant's forecast hour. A stage not
        // reached there is ase-plant's forecast to fix; a dead stand grows to no stage.
        const VivariumPatchView* patch = resources_.get_patch_at(jump.place);
        if (patch == nullptr) {
            return;  // the patch record follows in the next snapshot
        }
        const bool reached = patch->stage != GODOT_CODE_UNKNOWN && patch->stage >= jump.stage;
        const ::godot::String outcome =
            reached ? "reached" : (patch->dead ? "missed reason=dead" : "missed");
        note("debug jump " + outcome + " stage=" + ::godot::String::num_uint64(jump.stage) +
             " shown=" + ::godot::String::num_uint64(patch->stage) + at_words + " " + measure(jump.place));
    } else if (jump.eclipse_low != GODOT_ECLIPSE_NONE) {
        // THE LANDING OF AN ECLIPSE JUMP: the eclipse record ase-ephemeris shows at its predicted maximum.
        const VivariumSkyView& sky = resources_.get_sky();
        const VivariumEclipseView& eclipse = sky.eclipse[0];
        const bool standing = sky.known && sky.eclipses > 0u &&
                              eclipse.kind >= static_cast<double>(jump.eclipse_low) &&
                              eclipse.kind <= static_cast<double>(jump.eclipse_high);
        // THE SEARCH ON (D2256): a kind without an eclipse inside ase-ephemeris' horizon carries the end of the
        // horizon, and a reader that jumps by it searches on from there (CAUSA_ASE_EPHEMERIS,
        // EphemerisEclNextSystem). Without the eclipse standing, the jump takes ase-ephemeris' prediction made at
        // the new time and writes the clock there once more, up to debug.eclipse_hops times; the times stay
        // ase-ephemeris' and the view computes no orbit. Every landing without the eclipse is noted, so a
        // prediction that misses its own eclipse stays visible as ase-ephemeris' finding.
        const bool solar = jump.eclipse_high <= GODOT_ECLIPSE_SOLAR_MAX;
        double predicted_h = GODOT_HOUR_NONE;
        if (sky.known && sky.eclipse_nexts > 0u) {
            predicted_h = solar ? sky.eclipse_next[0].next_solar : sky.eclipse_next[0].next_lunar;
        }
        const ::godot::String kinds_word = "eclipse=" + ::godot::String::num_uint64(jump.eclipse_low) + ".." +
                                           ::godot::String::num_uint64(jump.eclipse_high);
        if (!standing && jump.hops < debug_.eclipse_hops && predicted_h > 0.0) {
            VivariumJump next;
            next.running = true;
            next.button = jump.button;
            next.asked_day = jump.asked_day;
            next.eclipse_low = jump.eclipse_low;
            next.eclipse_high = jump.eclipse_high;
            next.hops = jump.hops + 1u;
            if (seek_jump(next, GODOT_SEEK_FORM_NONE, predicted_h, GODOT_SEEK_FORM_NONE,
                          kinds_word + ::godot::String(" searched_on hop=") + ::godot::String::num_uint64(next.hops) +
                              ::godot::String(" predicted_h=") + ::godot::String::num(predicted_h, GODOT_LOG_DECIMALS) +
                              at_words)) {
                return;
            }
        }
        note(::godot::String("debug jump ") + (standing ? "reached " : "missed reason=no_eclipse ") + kinds_word +
             " hops=" + ::godot::String::num_uint64(jump.hops) + " kind=" + ::godot::String::num(eclipse.kind, 0) +
             " obscuration=" + ::godot::String::num(eclipse.obscuration, GODOT_LOG_DECIMALS) +
             " hours_to_max=" + ::godot::String::num(eclipse.hours_to_max, GODOT_LOG_DECIMALS) + at_words + " " +
             measure(selected_place_));
    } else if (jump.moonrise) {
        follow_moonrise(at_words);  // the moons in the camera's frustum (godot_vivarium_moonrise.cpp)
    } else {
        note("debug jump reached hour=" + ::godot::String::num(jump.hour, GODOT_LOG_DECIMALS) + at_words + " " +
             measure(selected_place_));
    }
    jump = VivariumJump{};
    node_->queue_redraw();
}

}  // namespace ase::adp::godot
