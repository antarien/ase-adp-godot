#pragma once

/**
 * ASE GODOT ADAPTER - HUD FRAME RATE
 *
 * @file        godot_hud_frame_rate.hpp
 * @brief       The frames per second of the running view over a short window, against the floor of the configuration
 * @description Operator order 2026-11-24: the player sees the frames per second in a corner of the screen while the
 *              game runs, and a rate under 30 is named as such. HudFrameRate counts frames over real time: it adds up
 *              the real distance between two frames until the window of hud_refresh_ms is full, and divides the frames
 *              of the window by the time they took. A frame longer than the window is a window of its own, so a stall
 *              shows in the number at once and is never averaged away by the frames before it.
 *
 *              THE FLOOR IS THE FRAME BUDGET, never a number of this file: view.frame_budget_ms of the start
 *              configuration (33.3 ms) is 30 frames per second, the same budget the order budget samples against. The
 *              number shown and the verdict are one reading: the rate is rounded to the whole number the player reads,
 *              and it stands under the floor when that number does. 29.97 of a 30 Hz screen reads 30 and is no finding.
 *
 *              THE TIME THE APP WAS AWAY IS NO FRAME. restart() drops the open window and the next frame's distance:
 *              the view calls it when it returns from the background, and the span it slept is not counted as a frame
 *              that took that long.
 *
 *              NO GODOT TYPE, so the host tests drive it with any series of frame distances. UiFrameRate
 *              (godot_hud_ui_frame_rate.hpp) draws the number.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
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

#include <cstdint>

namespace ase::adp::godot {

/** The frames per second of the last full window and their verdict against the floor. */
class HudFrameRate {
public:
    /**
     * The length of a window in seconds and the frame budget in milliseconds. False, with nothing changed, when one
     * of them is not a positive finite number or the budget leaves no whole frame a second.
     */
    bool configure(double window_s, double frame_budget_ms);

    /**
     * One frame, `elapsed_s` real seconds after the one before. True when this frame filled the window: the rate, the
     * number and the verdict below are new. A distance that is no positive finite number counts as no frame.
     */
    bool frame(double elapsed_s);

    /** The open window and the distance of the next frame are dropped; the last reading stays. */
    void restart();

    double  rate() const { return rate_; }        // frames per second of the last full window
    int32_t shown() const { return shown_; }      // the whole number the player reads
    int32_t floor_rate() const { return floor_; } // the whole number it may not fall under
    bool    known() const { return known_; }      // a window was full once
    bool    under() const { return under_; }      // the number stands under the floor
    bool    changed() const { return changed_; }  // the last full window brought another number than the one before
    bool    crossed() const { return crossed_; }  // the last full window brought another verdict, or the first one

private:
    double  window_s_ = 0.0;   // seconds a window lasts at least
    double  span_s_ = 0.0;     // seconds of the open window
    double  rate_ = 0.0;
    int32_t frames_ = 0;       // frames of the open window
    int32_t shown_ = 0;
    int32_t floor_ = 0;
    bool    fresh_ = true;     // the next frame starts the count and brings no time of its own
    bool    known_ = false;
    bool    under_ = false;
    bool    changed_ = false;
    bool    crossed_ = false;
};

}  // namespace ase::adp::godot
