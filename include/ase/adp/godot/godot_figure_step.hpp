#pragma once

/**
 * ASE GODOT ADAPTER - FIGURE STEP
 *
 * @file        godot_figure_step.hpp
 * @brief       The way a frame draws a figure between two fixed steps of its port, and the share of its turn a frame closes
 * @description The body port moves a figure once per fixed step of the Kinetic tier (Dynamics at 1/30 s, the
 *              accumulator loop of TickScheduler::tick), the frame comes at its own rate. A body set to the place
 *              of the port stands still for some frames and jumps on the next one; above the step rate that is
 *              the stutter of a figure that walks. Between two moves the frame therefore draws the body on the way
 *              from the place the port gave before to the place it gives now, by the share of the span between
 *              the two moves that has passed (the render interpolation of a fixed time step). The picture then
 *              trails the port by one step at most and is as smooth as the frames are. A move that comes later
 *              than a frame of 20 Hz after the one before is drawn at once: under that frame rate every frame
 *              carries a step of its own, and the blend would only add a frame of lag.
 *
 *              A NEW WAY STARTS AT THE PLACE BEFORE, never where the body was drawn: a way that started from the
 *              drawn place would begin each step where the last blend broke off and trail further behind with
 *              every step.
 *
 *              THE TURN IS FREE OF THE FRAME RATE. The body closes the share 1 - e^(-rate dt) of its open course
 *              per frame: two frames of dt/2 close as much as one frame of dt, so the turn takes the same time at
 *              20 and at 120 frames per second. A share of dt * rate, capped at 1, does not: it turns a figure
 *              faster at few frames and snaps it at a long one.
 *
 *              No Godot type stands here, so the way is tested without a Godot process
 *              (tests/test_godot_figure_step.cpp); the node of a figure keeps it as meta (godot_figure_way.hpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_030
 * @category    render/pipeline/stage
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

namespace ase::adp::godot {

/** The way one figure is drawn on between the last two moves of its port, in the scene frame. */
struct FigureStepWay {
    double from_x = 0.0;  // the place the port gave before its last move, where the drawn way starts
    double from_y = 0.0;
    double from_z = 0.0;
    double to_x = 0.0;    // the place the port gave last, where the drawn way ends
    double to_y = 0.0;
    double to_z = 0.0;
    double since = 0.0;   // seconds of frames since the port moved the figure last
    double span = 0.0;    // seconds between the last two moves of the port, the span the way is drawn over
};

/**
 * Takes the place `x`, `y`, `z` the port gives in a frame of `dt` seconds into `way`; `fresh` for a figure
 * without a way yet, which stands at the place at once. A place other than the last one starts a new way.
 */
void figure_step_take(FigureStepWay& way, double x, double y, double z, double dt, bool fresh);

/** The share of the way the frame draws: 0 at the place before, 1 at the place now. */
double figure_step_along(const FigureStepWay& way);

/** The share of the open course a frame of `dt` seconds closes at `rate` per second: 1 - e^(-rate dt), 0 to 1. */
double figure_turn_closed(double dt, double rate);

}  // namespace ase::adp::godot
