/**
 * ASE ADAPTER TESTS - THE FRAMES PER SECOND OF THE HUD, WITHOUT GODOT
 *
 * @file        test_godot_hud_frame_rate.cpp
 * @design      DSGN_067
 * @brief       Holds the number the player reads and its verdict under 30 against series of frame distances
 * @description Operator order 2026-11-24. The window and the budget are the ones of the app: hud_refresh_ms 250 and
 *              view.frame_budget_ms 33.3. Cases: 60, 30 and 29 frames a second as steady series, the number of a slow
 *              host at 2 frames a second, the edge from over to under and back named once each, a stall that shows in
 *              the very next reading, the time the app was away that counts as no frame, a number that changes only
 *              when the reading does, distances that are no time, and a configuration that cannot judge.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_hud_frame_rate.hpp>

#include <limits>

namespace ase::adp::godot {

namespace {

constexpr double kWindow = 0.25;   // hud_refresh_ms of SHA's design tokens, seconds
constexpr double kBudget = 33.3;   // view.frame_budget_ms of vivarium_start.json

HudFrameRate app_rate() {
    HudFrameRate rate;
    CHECK(rate.configure(kWindow, kBudget));
    return rate;
}

/** Counts over a steady series: the readings, the numbers that changed and the verdicts that changed. */
struct Run {
    int readings = 0;
    int changes = 0;
    int edges = 0;
};

Run run(HudFrameRate& rate, double per_second, double seconds) {
    Run out;
    const int frames = static_cast<int>(per_second * seconds);
    for (int i = 0; i < frames; ++i) {
        if (rate.frame(1.0 / per_second)) {
            ++out.readings;
            out.changes += rate.changed() ? 1 : 0;
            out.edges += rate.crossed() ? 1 : 0;
        }
    }
    return out;
}

}  // namespace

TEST_CASE("the floor is the frame budget of the configuration") {
    const HudFrameRate rate = app_rate();
    CHECK(rate.floor_rate() == 30);
    CHECK_FALSE(rate.known());
}

TEST_CASE("sixty frames a second read 60, over the floor, one number and one verdict") {
    HudFrameRate rate = app_rate();
    const Run seen = run(rate, 60.0, 10.0);
    CHECK(seen.readings >= 30);
    CHECK(rate.shown() == 60);
    CHECK_FALSE(rate.under());
    CHECK(seen.changes == 1);
    CHECK(seen.edges == 1);
}

TEST_CASE("thirty frames a second are no finding, twenty-nine are one") {
    HudFrameRate at = app_rate();
    run(at, 30.0, 10.0);
    CHECK(at.shown() == 30);
    CHECK_FALSE(at.under());

    HudFrameRate ntsc = app_rate();
    run(ntsc, 29.97, 10.0);
    CHECK(ntsc.shown() == 30);
    CHECK_FALSE(ntsc.under());

    HudFrameRate below = app_rate();
    run(below, 29.0, 10.0);
    CHECK(below.shown() == 29);
    CHECK(below.under());
}

TEST_CASE("a slow host reads its own two frames, each frame a window of its own") {
    HudFrameRate rate = app_rate();
    const Run seen = run(rate, 2.0, 10.0);
    CHECK(seen.readings == 19);  // twenty frames, the first one starts the count
    CHECK(rate.shown() == 2);
    CHECK(rate.under());
    CHECK(seen.edges == 1);
}

TEST_CASE("the fall under the floor and the return are one edge each") {
    HudFrameRate rate = app_rate();
    run(rate, 60.0, 4.0);
    const Run falls = run(rate, 20.0, 4.0);
    CHECK(rate.under());
    CHECK(falls.edges == 1);
    const Run rises = run(rate, 60.0, 4.0);
    CHECK_FALSE(rate.under());
    CHECK(rises.edges == 1);
}

TEST_CASE("a stall shows in the reading it ends in") {
    HudFrameRate rate = app_rate();
    run(rate, 60.0, 4.0);
    CHECK(rate.frame(1.5));
    CHECK(rate.under());
    CHECK(rate.shown() < 30);
}

TEST_CASE("the time the app was away counts as no frame") {
    HudFrameRate rate = app_rate();
    run(rate, 60.0, 4.0);
    rate.restart();
    CHECK_FALSE(rate.frame(3600.0));
    CHECK(rate.shown() == 60);
    const Run back = run(rate, 60.0, 4.0);
    CHECK(back.edges == 0);
    CHECK_FALSE(rate.under());
}

TEST_CASE("a distance that is no time counts as no frame") {
    HudFrameRate rate = app_rate();
    run(rate, 60.0, 4.0);
    CHECK_FALSE(rate.frame(0.0));
    CHECK_FALSE(rate.frame(-1.0));
    CHECK_FALSE(rate.frame(std::numeric_limits<double>::quiet_NaN()));
    CHECK_FALSE(rate.frame(std::numeric_limits<double>::infinity()));
    run(rate, 60.0, 4.0);
    CHECK(rate.shown() == 60);
    CHECK_FALSE(rate.under());
}

TEST_CASE("a configuration that cannot judge is refused and judges nothing") {
    HudFrameRate rate;
    CHECK_FALSE(rate.configure(0.0, kBudget));
    CHECK_FALSE(rate.configure(kWindow, 0.0));
    CHECK_FALSE(rate.configure(kWindow, 1.0e9));
    CHECK_FALSE(rate.configure(std::numeric_limits<double>::quiet_NaN(), kBudget));
    CHECK_FALSE(rate.frame(1.0));
    CHECK_FALSE(rate.frame(1.0));
    CHECK_FALSE(rate.known());
}

}  // namespace ase::adp::godot
