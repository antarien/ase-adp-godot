#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM WATCH RECORDS
 *
 * @file        godot_vivarium_watch.hpp
 * @brief       The records the Vivarium view keeps while it measures itself: the measurement run and the ground watch
 * @description Two records of the view's own time, read and written by godot_vivarium_measure.cpp (the measurement
 *              run of acceptance A10), godot_vivarium_flatten.cpp (the ground watch after a ground act),
 *              godot_vivarium_node.cpp (the phase times of every _process) and godot_vivarium_draw.cpp (the canvas
 *              time of the HUD). They stand apart from godot_vivarium_node.hpp because only those measuring units
 *              change them: a new share of the frame recompiles the view's class through this header, never the
 *              records of the configuration, the layout or the debug mode.
 *
 *              NOTHING HERE STEERS THE VIEW: a field of these records is read back only into a log line. A frame
 *              that measures itself draws and advances exactly as one that does not.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-13
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_cycle_measure.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/**
 * One measurement run (acceptance A10): real time counts down the warm-up, then the recording,
 * and every frame in the recording stores its host time (advance + snapshots) and its whole
 * _process interval. The block is written ONCE, when the recording is complete - nothing is
 * logged per frame while it runs.
 */
struct VivariumMeasurement {
    bool                  active = false;       // requested at launch and not yet reported
    bool                  recording = false;    // warm-up over, samples are stored
    double                warmup_left = 0.0;    // seconds of warm-up still to run
    double                record_left = 0.0;    // seconds of recording still to run
    uint64_t              last_usec = 0;        // Time::get_ticks_usec of the previous frame, 0 = none
    std::vector<uint32_t> host_usec;            // per recorded frame: advance + snapshots
    std::vector<uint32_t> frame_usec;           // per recorded frame: the interval since the previous frame
    uint64_t              pss_start_kib = 0;    // PSS when the recording began
    bool                  pss_start_read = false;
    uint32_t              catch_ups = 0;        // returns from the background that fell into the recording
    uint32_t              boots_at_start = 0;   // host boots counted when the recording began
    CycleMeasure          cycles;               // the line of one game day (03.6), cursors since the recording began
    bool                  cycles_ready = false; // units, schedules and organ ports named, cursors set
    uint64_t              cycle_dawns = 0;      // GameDawn runs the host had counted when the current cycle began
    bool                  cycle_counted = false; // cycle_dawns was read from the running host
};

/**
 * The frames after one ground act (P0690): the act starts the watch, every _process interval of the next
 * GODOT_GROUND_WATCH_FRAMES frames is held against the longest so far, and the watch writes ONE line when its
 * frames are counted or the next act begins. Runs in every mode, apart from the measurement run. Between acts it
 * keeps the longest interval of the last GODOT_GROUND_WATCH_FRAMES frames, so the line holds the frames after an
 * act against the frames before it, and it keeps the longest host advance among the watched frames, so the line
 * says whether the host's tick or the drawing filled the interval. Its own cycle line (03.6, godot_cycle_measure.hpp),
 * begun at the act and written with the watch's line, names the compute time of every unit and schedule over the
 * watched frames, so the line also says WHICH unit filled the host's share. The HUD's canvas time of every frame
 * is kept apart from the rest after the _process, so the line says whether the canvas or Godot's own steps
 * (physics, render, input) filled that rest.
 */
struct VivariumGroundWatch {
    std::string op;                   // the act watched: lower, raise or flatten; empty = no watch runs
    uint32_t    frames_left = 0;      // intervals still to watch
    uint32_t    frames = 0;           // intervals watched so far
    uint64_t    act_usec = 0;         // the act's own time in the host (the port call)
    uint64_t    max_usec = 0;         // the longest interval watched so far
    uint64_t    host_max_usec = 0;    // the longest host advance among the watched frames
    uint64_t    before_usec = 0;      // the longest interval of the frames before the act
    uint64_t    span_max_usec = 0;    // between acts: the longest interval of the span running now
    uint64_t    span_prev_usec = 0;   // between acts: the longest interval of the span before it
    uint32_t    span_frames = 0;      // between acts: intervals counted in the span running now
    uint64_t    last_usec = 0;        // Time::get_ticks_usec at the start of the previous frame, 0 = none
    CycleMeasure units;               // the compute time per unit and schedule since the act, cursors set at the act
    bool        units_ready = false;  // units, schedules and organ ports named, cursors set at the act
    uint64_t    units_begin_usec = 0; // the time the act's frame spent setting those cursors
    uint64_t    follow_max_usec = 0;  // the longest share of the follow_* steps before the scene among the watched frames
    uint64_t    scene_max_usec = 0;   // the longest follow_scene among the watched frames
    uint64_t    hud_max_usec = 0;     // the longest tick_hud among the watched frames
    uint64_t    process_max_usec = 0; // the longest whole _process among the watched frames
    uint64_t    rest_max_usec = 0;    // the longest interval share after the previous _process: drawing and rendering
    uint64_t    last_process_usec = 0; // the whole _process of the previous frame, 0 = none
    uint64_t    draw_max_usec = 0;    // the longest HUD canvas drawing among the watched frames
    uint64_t    last_draw_usec = 0;   // the HUD canvas drawing since the previous frame's start, 0 = none drawn
    uint64_t    last_cpu_usec = 0;    // the main thread's CPU time at the start of the previous frame, 0 = none
    uint64_t    cpu_at_max_usec = 0;  // the main thread's CPU time inside the longest watched interval
    uint64_t    draw_cpu_max_usec = 0; // the most CPU time one HUD canvas drawing took among the watched frames
    uint64_t    last_draw_cpu_usec = 0; // the CPU time of the HUD canvas drawing since the previous frame's start
};

/**
 * The CPU time the calling thread has spent, in microseconds (CLOCK_THREAD_CPUTIME_ID). Held against the wall clock
 * of the same span it says how much of the span the thread computed and how much it waited for a core (P0690: on a
 * host whose load exceeds its cores a frame's interval grows while its work stays the same).
 */
uint64_t thread_cpu_usec();

}  // namespace ase::adp::godot
