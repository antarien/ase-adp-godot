/**
 * ASE ADAPTER TESTS - THE WAY OF A FIGURE BETWEEN TWO FIXED STEPS WITHOUT GODOT
 *
 * @file        test_godot_figure_step.cpp
 * @design      DSGN_030
 * @brief       A figure walks smooth between the steps of its port and turns in the same time at every frame rate
 * @description B0943: the port moves a figure once per fixed step of Dynamics (1/30 s), the frame comes at its own
 *              rate (godot_figure_step.hpp). Held here: a frame between two moves draws the share of the way that
 *              has passed, a frame at a move draws the place before, the way of the next move starts at the
 *              place before and never at the drawn one, a move later than a frame of 20 Hz is drawn at once, and
 *              the share of the turn is the same over two half frames as over one whole frame.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101588 - move player figure over ground
 * @actor       SS/PC
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_figure_step.hpp>

namespace {

constexpr double kStep = 1.0 / 30.0;     // one fixed step of Dynamics, the rate the port moves a figure at
constexpr double kFrame = 1.0 / 120.0;   // a frame of 120 Hz, four frames per step
constexpr double kSlowFrame = 0.1;       // a frame of 10 Hz, under the span the way is drawn over
constexpr double kTurnRate = 8.0;        // a rate of the turn per second, as SHA's hud_figure_body_turn_rate gives one

}  // namespace

using ase::adp::godot::figure_step_along;
using ase::adp::godot::figure_step_take;
using ase::adp::godot::figure_turn_closed;
using ase::adp::godot::FigureStepWay;

TEST_CASE("usecase:101588: a new figure stands at the place of its port at once") {
    FigureStepWay way;
    figure_step_take(way, 2.0, 0.0, 3.0, kFrame, true);
    CHECK(figure_step_along(way) == doctest::Approx(1.0));
    CHECK(way.from_x == doctest::Approx(2.0));
    CHECK(way.to_z == doctest::Approx(3.0));
}

TEST_CASE("usecase:101588: between two moves of the port a frame draws the share of the way that has passed") {
    FigureStepWay way;
    figure_step_take(way, 0.0, 0.0, 0.0, kFrame, true);
    // the first move after a step: the way runs from 0 to 1 over the step since the place was laid
    for (int frame = 0; frame < 3; ++frame) {
        figure_step_take(way, 0.0, 0.0, 0.0, kFrame, false);
    }
    figure_step_take(way, 1.0, 0.0, 0.0, kFrame, false);
    CHECK(way.span == doctest::Approx(kStep));
    CHECK(figure_step_along(way) == doctest::Approx(0.0));  // the frame of the move draws the place before
    figure_step_take(way, 1.0, 0.0, 0.0, kFrame, false);
    CHECK(figure_step_along(way) == doctest::Approx(0.25));
    figure_step_take(way, 1.0, 0.0, 0.0, kFrame, false);
    CHECK(figure_step_along(way) == doctest::Approx(0.5));
    figure_step_take(way, 1.0, 0.0, 0.0, kFrame, false);
    CHECK(figure_step_along(way) == doctest::Approx(0.75));
}

TEST_CASE("usecase:101588: the next way starts at the place before, never where the blend broke off") {
    FigureStepWay way;
    figure_step_take(way, 0.0, 0.0, 0.0, kFrame, true);
    for (int frame = 0; frame < 3; ++frame) {
        figure_step_take(way, 0.0, 0.0, 0.0, kFrame, false);
    }
    figure_step_take(way, 1.0, 0.0, 0.0, kFrame, false);
    figure_step_take(way, 1.0, 0.0, 0.0, kFrame, false);  // drawn at a quarter of the way to 1
    figure_step_take(way, 2.0, 0.0, 0.0, kFrame, false);  // the port moves on before the blend reached 1
    CHECK(way.from_x == doctest::Approx(1.0));
    CHECK(way.to_x == doctest::Approx(2.0));
    CHECK(figure_step_along(way) == doctest::Approx(0.0));
}

TEST_CASE("usecase:101588: a move later than a frame of 20 Hz is drawn at once") {
    FigureStepWay way;
    figure_step_take(way, 0.0, 0.0, 0.0, kSlowFrame, true);
    figure_step_take(way, 0.5, 0.0, 0.0, kSlowFrame, false);
    CHECK(way.span == doctest::Approx(kSlowFrame));
    CHECK(figure_step_along(way) == doctest::Approx(1.0));
}

TEST_CASE("usecase:101588: the share of the turn is free of the frame rate") {
    const double whole = figure_turn_closed(kStep, kTurnRate);
    const double half = figure_turn_closed(kStep / 2.0, kTurnRate);
    // two half frames close 1 - (1 - half)^2 of the open course, as much as one whole frame
    CHECK(1.0 - (1.0 - half) * (1.0 - half) == doctest::Approx(whole).epsilon(1.0e-5));
    CHECK(whole > 0.0);
    CHECK(whole < 1.0);
    CHECK(figure_turn_closed(0.0, kTurnRate) == doctest::Approx(0.0));
    CHECK(figure_turn_closed(10.0, kTurnRate) == doctest::Approx(1.0).epsilon(1.0e-5));
}
