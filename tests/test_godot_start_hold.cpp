/**
 * ASE ADAPTER TESTS - THE STEP THAT HOLDS THE START SCREEN
 *
 * @file        test_godot_start_hold.cpp
 * @design      DSGN_066
 * @brief       Pins the one order in which the steps of the initialisation hold the start screen
 * @description B0357, PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.9: the start screen holds until the figure file is
 *              loaded, and its log line names the step that holds it (held_by=). start_hold_step chooses that
 *              step for the log, the word of the bar and the hold itself (godot_vivarium_clock.cpp), so this
 *              test pins the order of the bar: patches, brooks, scene, sky_clock, figure, jump, render. Pure
 *              computation, no Godot type.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.26.00026 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_start_hold.hpp>

#include <string>

using namespace ase::adp::godot;

namespace {

// a reading in which every step stands done: no figure loads, no jump runs
StartHoldReading all_done() {
    StartHoldReading reading;
    reading.patches_announced = true;
    reading.brooks_carried = true;
    reading.scene_built = true;
    reading.sky_and_clock = true;
    return reading;
}

}  // namespace

TEST_CASE("B0357: a fresh boot is held by the patches, a finished one by the quiet frames alone") {
    CHECK(start_hold_step(StartHoldReading{}) == GODOT_HOLD_PATCHES);
    CHECK(start_hold_step(all_done()) == GODOT_HOLD_RENDER);
}

TEST_CASE("B0357: each step holds the screen in the order of the bar") {
    StartHoldReading reading = all_done();
    reading.jumping = true;
    CHECK(start_hold_step(reading) == GODOT_HOLD_JUMP);
    reading.figure_loading = true;
    CHECK(start_hold_step(reading) == GODOT_HOLD_FIGURE);  // the figure file holds before the jump
    reading.sky_and_clock = false;
    CHECK(start_hold_step(reading) == GODOT_HOLD_SKY_CLOCK);
    reading.scene_built = false;
    CHECK(start_hold_step(reading) == GODOT_HOLD_SCENE);
    reading.brooks_carried = false;
    CHECK(start_hold_step(reading) == GODOT_HOLD_BROOKS);
    reading.patches_announced = false;
    CHECK(start_hold_step(reading) == GODOT_HOLD_PATCHES);
}

TEST_CASE("B0357: the figure file alone holds a screen whose every other step stands") {
    StartHoldReading reading = all_done();
    reading.figure_loading = true;
    CHECK(start_hold_step(reading) == GODOT_HOLD_FIGURE);
    reading.figure_loading = false;
    CHECK(start_hold_step(reading) == GODOT_HOLD_RENDER);
}

TEST_CASE("B0357: the log names every step with the word of held_by=") {
    CHECK(std::string(start_hold_name(GODOT_HOLD_PATCHES)) == "patches");
    CHECK(std::string(start_hold_name(GODOT_HOLD_BROOKS)) == "brooks");
    CHECK(std::string(start_hold_name(GODOT_HOLD_SCENE)) == "scene");
    CHECK(std::string(start_hold_name(GODOT_HOLD_SKY_CLOCK)) == "sky_clock");
    CHECK(std::string(start_hold_name(GODOT_HOLD_FIGURE)) == "figure");
    CHECK(std::string(start_hold_name(GODOT_HOLD_JUMP)) == "jump");
    CHECK(std::string(start_hold_name(GODOT_HOLD_RENDER)) == "render");
    // a value beyond the steps names the last one, never an address past the table
    CHECK(std::string(start_hold_name(GODOT_HOLD_STEPS)) == "render");
}
