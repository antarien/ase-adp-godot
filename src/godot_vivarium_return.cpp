/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW: THE RETURN TO THE GARDEN
 *
 * @file        godot_vivarium_return.cpp
 * @brief       VivariumView - the platform's background and foreground, the return to the garden and its catch-up
 * @design      DSGN_066
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.6 point 2 and V6, PLAN_ASE_VIVARIUM_PHASE_06_HUD row A08: the
 *              keeper leaves the app and comes back. THE WORLD DOES NOT STOP (PLAN 01.3 point 4): the background is
 *              logged as a fact, nothing is held; the next frame brings the time it lasted, follow_catch_up logs
 *              its return and the seconds until ase-time laid it.
 *
 *              THE HUD OPENS ON THE TRAIL: a front that follows a background is the keeper coming back
 *              (garden_came_back), and the HUD opens on the mode it starts in (hud.modes.start, the arrival with
 *              its trail), as at the start of the app. The first front of a boot follows no background and changes
 *              nothing; the view already starts in arrive.
 *
 *              ONE CLASS, ITS BEHAVIOUR BY RESPONSIBILITY (godot_vivarium_node.cpp): this file carries only the
 *              return; the decisions stand in godot_garden_return.cpp, which carries no Godot type.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    time/period/cycle
 * @usecase     usecase:100390 - return to the garden
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_garden_return.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <godot_cpp/classes/node.hpp>

#include <cstdint>

namespace ase::adp::godot {

void VivariumView::notification(int32_t what) {
    if (in_editor()) {
        return;
    }
    // THE WORLD DOES NOT STOP (PLAN 01.3 point 4): the background is logged as a fact, nothing is
    // held. The next frame brings the time it lasted, and follow_catch_up logs its return.
    bool came_back = false;
    if (what == ::godot::Node::NOTIFICATION_APPLICATION_PAUSED) {
        note("background reason=application " + measure(GODOT_NO_PLACE));
        came_back = garden_came_back(garden_return_, false);
    } else if (what == ::godot::Node::NOTIFICATION_APPLICATION_RESUMED) {
        note("foreground reason=application " + measure(GODOT_NO_PLACE));
        came_back = garden_came_back(garden_return_, true);
    } else if (what == ::godot::Node::NOTIFICATION_APPLICATION_FOCUS_OUT) {
        note("background reason=focus " + measure(GODOT_NO_PLACE));
        came_back = garden_came_back(garden_return_, false);
    } else if (what == ::godot::Node::NOTIFICATION_APPLICATION_FOCUS_IN) {
        note("foreground reason=focus " + measure(GODOT_NO_PLACE));
        came_back = garden_came_back(garden_return_, true);
        // the platform's reduced-motion setting can change while the app lies in the background (row T09)
        read_hud_still();
    }
    // THE HUD OPENS ON THE TRAIL (usecase:100390): fill_hud sees the new mode and forces the render, draw_hud writes
    // the edge "hud mode=arrive" once its frame stands; this line names the cause of that edge.
    if (came_back && hud_live()) {
        hud_.return_to_garden();
        note("garden return mode=" + ::godot::String(hud_.mode().c_str()) + " " + measure(GODOT_NO_PLACE));
    }
}

void VivariumView::follow_catch_up(double elapsed_s, double now_s) {
    // The return to the garden (godot_garden_return.hpp) decides what this frame says: the frame that
    // noticed a span the app spent away, and the frame in which its backlog ran dry with the seconds
    // until ase-time laid the filed real position (V6). This function only writes the two lines. The
    // long frame is one tick (frame_ticks=1) and one filed position; real_laid_s is the position the
    // clock record shows laid. measure() carries steps= (the Dissemination runs), and a key standing
    // twice in one line keeps only its last value for every key=value reader.
    const GardenReturnFrame frame =
        garden_return_frame(garden_return_, now_s, elapsed_s, resources_.backlog_seconds());
    const VivariumClockView& clock = resources_.get_clock();
    if (frame.noticed) {
        measurement_.catch_ups += measurement_.recording ? 1u : 0u;
        note("catch_up elapsed_s=" + ::godot::String::num(elapsed_s, GODOT_LOG_DECIMALS) +
             " frame_ticks=" + ::godot::String::num_uint64(resources_.last_steps()) +
             " backlog_s=" + ::godot::String::num(resources_.backlog_seconds(), GODOT_LOG_DECIMALS) +
             " real_laid_s=" + ::godot::String::num(clock.real_laid_s, GODOT_LOG_DECIMALS) + " " +
             measure(GODOT_NO_PLACE));
    }
    if (frame.caught_up) {
        note("caught_up catch_up_s=" + ::godot::String::num(frame.catch_up_s, GODOT_LOG_DECIMALS) +
             " real_laid_s=" + ::godot::String::num(clock.real_laid_s, GODOT_LOG_DECIMALS) + " " +
             measure(GODOT_NO_PLACE));
    }
}

}  // namespace ase::adp::godot
