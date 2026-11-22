#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM DEBUG RECORDS
 *
 * @file        godot_vivarium_debug.hpp
 * @brief       The records of the Vivarium's debug mode: the time lapse, the jump, the date entry, the relief, the clock constants
 * @description The debug mode of the Vivarium view (modes.debug of the start configuration) is drawn and
 *              answered by godot_vivarium_debug.cpp, godot_vivarium_jump.cpp and godot_vivarium_clock.cpp;
 *              this header holds what those units read from the section "debug" and what they keep while a request runs. It
 *              stands apart from godot_vivarium_node.hpp because only the debug units change it: a new
 *              button kind or a field of the running jump recompiles the view's class, never the
 *              records of the configuration, the layout or the measurement.
 *
 *              TWO CASES OF TIME, NOTHING ELSE (operator order of 2026-11-06): the time lapse runs the
 *              host's clock at a multiple of ase-time's base pace, the factor travels through the pace
 *              port and ase-pl-flora multiplies it with the base pace read from the star
 *              (usecase:101120 clock the growth cycle). The jump sets the clock to a target - a stage
 *              of the selected patch or an hour of the game day - behind the start screen, through
 *              the seek port: ase-time gives the new time, and every module takes it from the star
 *              and computes its own state at it, as on a start of the app at that time. Nothing is
 *              replayed, and the pace stays as it was. Both run on the client's own clock; no server
 *              clock moves.
 *
 *              THE RELIEF (usecase:6018 derive terrain heights) asks the elevation port for the relief
 *              of the whole ground at runtime; the seed keeps the shape of the landscape.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    time/period/cycle
 * @usecase     usecase:100390 - return to the garden
 * @actor       Plant Keeper (PLNTKEEPER)
 * @created     2026-11-06
 * @modified    2026-11-19
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/host/types.hpp>
#include <ase/adp/godot/patch/types.hpp>

#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

/**
 * One time lapse button of the debug mode (debug.buttons of the start configuration), the first of the two
 * cases: it asks this host's game clock to run at a multiple of ase-time's base pace. The view sends the
 * factor only; the base pace and the product are ase-time's, multiplied by ase-pl-flora on the star. The
 * clock runs faster, with no target and no end.
 */
struct VivariumDebugButton {
    ::godot::Rect2   rect;
    ::godot::String  label;
    ::godot::String  icon;                     // the glyph of its ICON_* in SHA's icon table, drawn before the label
    ::godot::Vector2 icon_offset;              // the offset Icon.tsx gives that glyph, CSS pixels at debug.icon_web_px
    double          factor = GODOT_PACE_NONE;   // the multiple of the base pace the button asks; GODOT_PACE_NONE: the default
};

/**
 * One jump button of the debug mode (debug.jumps of the start configuration), the second of the two cases:
 * the clock is set to a target behind the start screen, and every module computes its state at the new
 * time; the pace stays as it was before. The target is a stage of the selected patch, an hour of the game
 * day or a SURA date whole VECTUM ahead - the same case, only the recognition of the target differs.
 */
struct VivariumDebugJump {
    ::godot::Rect2   rect;
    ::godot::String  label;
    ::godot::String  icon;                     // the glyph of its ICON_* in SHA's icon table, empty: the label alone
    ::godot::Vector2 icon_offset;              // the offset Icon.tsx gives that glyph, CSS pixels at debug.icon_web_px
    uint8_t         stage = GODOT_STAGE_NONE;   // the stage the selected patch jumps to; NONE: the jump names an hour
    double          hour = GODOT_HOUR_NONE;     // the hour of the game day the clock jumps to; NONE: the jump names a stage
    double          years = GODOT_YEARS_NONE;   // VECTUM the date jump goes ahead, month, day and hour kept; NONE: a stage or an hour
    uint8_t         eclipse_low = GODOT_ECLIPSE_NONE;   // lowest ase-ephemeris ECLIPSE_* kind an eclipse jump lands on; NONE: no eclipse jump
    uint8_t         eclipse_high = GODOT_ECLIPSE_NONE;  // highest ECLIPSE_* kind it lands on (solar 1..2, lunar 3..4)
    bool            moonrise = false;           // the jump goes to the next rise of a moon into the camera's frame; false: another target
};

/**
 * The jump that runs (a VivariumDebugJump asked and granted). EVERY JUMP IS ONE WRITE (CAUSA_ASE_TIME, "The
 * Seek Contract"): its target goes once into ase-time's clock, and every module computes its own state at the
 * new time in its own schedule. The view lays no target and compares no clock: it sends the target in the
 * form the source names it, and ase-time lays it from its own clock. An hour jump sends the hour of the game
 * day (ase-time takes its next occurrence), a stage jump the absolute game hour ase-plant forecasts, an
 * eclipse or a moonrise jump the game hours ahead ase-ephemeris forecasts - all through the seek port. A date
 * jump sends its SURA date through the date port (ase-calendar computes its game hour, ase-time sets its
 * clock there), target_year to target_hour. Every request carries the number seq, and ase-time judges it on
 * the clock record (TimeClkSeekCalcSystem, VivariumClockView::seek_seq and seek_order): ahead or at, the
 * clock stands at the target; behind, a date of the free entry builds the host anew and seeks again from the
 * start clock of the run (rewound, ahead false); refused, the jump ends with its reason. Ahead or at, the jump
 * ends on the verdict (running falls, the start screen lifts on the criteria of a start), and once the host ran
 * settle_s simulated seconds at the new time since the write (sought_run_s), follow_debug notes the landing -
 * the stage shown, the eclipse kind of the sky port, the moons in the camera's frame - and clears the jump. The
 * view forecasts nothing; the times are the modules'.
 */
struct VivariumJump {
    bool     running = false;                  // a jump runs
    uint32_t button = UINT32_MAX;              // index of its button in debug.jumps, drawn with the wider outline
    uint8_t  stage = GODOT_STAGE_NONE;         // the stage it ends at, NONE: an hour jump
    uint32_t place = GODOT_NO_PLACE;           // the place whose patch a stage jump watches
    double   hour = GODOT_HOUR_NONE;           // the hour of the game day an hour jump ends at
    double   seq = 0.0;                        // number of the request the jump wrote last, 0: none written yet
    double   target_year = GODOT_HOUR_NONE;    // VECTUM a date jump ends at, below 0: not a date jump
    double   asked_day = 0.0;                  // the game day the jump was asked on
    uint8_t  eclipse_low = GODOT_ECLIPSE_NONE;  // lowest ECLIPSE_* kind an eclipse jump lands on, NONE: no eclipse jump
    uint8_t  eclipse_high = GODOT_ECLIPSE_NONE; // highest ECLIPSE_* kind it lands on
    uint32_t hops = 0u;                         // searches on of an eclipse jump past a landing without its eclipse, below debug.eclipse_hops (D2256)
    double   sought_run_s = GODOT_HOUR_NONE;    // stage seconds the host had ticked when the jump last set the clock (stage_seconds), below 0: not set yet
    double   target_month = GODOT_HOUR_NONE;    // LUNBREX a date jump ends at
    double   target_month_day = GODOT_HOUR_NONE;  // TUMBRAE a date jump ends at
    double   target_hour = GODOT_HOUR_NONE;     // HORAX a date jump ends at
    bool     ahead = true;                      // a date jump lies ahead of the clock; false: ase-time judged it behind, the host was built anew for it
    bool     rewound = false;                   // a date jump back: the host was built anew, the seek ahead from its start clock is owed
    bool     entry = false;                    // the jump was asked from the free date entry (debug.date)
    bool     moonrise = false;                  // a moonrise jump: it lands at ase-ephemeris' forecast of the next rise
};

/**
 * One field of the free date entry (debug.date.fields): a part of the SURA date the jump goes to - the VECTUM,
 * the LUNBREX, the TUMBRAE or the whole HORAX - drawn with its label and its value.
 */
struct VivariumDebugDateField {
    ::godot::Rect2  rect;
    ::godot::String label;
    uint8_t         part = GODOT_DATE_PART_NONE;  // GODOT_DATE_PART_* the field shows
};

/**
 * One step of the free date entry (debug.date.steps): it moves one part of the entry by whole units. The
 * LUNBREX, the TUMBRAE and the HORAX turn round within ase-calendar's months of a VECTUM and days of a LUNBREX
 * and ase-time's hours of a day, all three from the clock record; the VECTUM stops at the first of the run.
 */
struct VivariumDebugDateStep {
    ::godot::Rect2  rect;
    ::godot::String label;
    uint8_t         part = GODOT_DATE_PART_NONE;  // GODOT_DATE_PART_* the step moves
    double          delta = 0.0;                  // whole units it moves the part, below 0: back
};

/**
 * One key of the typed date entry (debug.date.keys): it appends its digit to the part a tap on a field picked.
 * The first key after the pick starts the value afresh, so a LUNBREX is two taps, not a walk of steps.
 */
struct VivariumDebugDateKey {
    ::godot::Rect2  rect;
    ::godot::String label;
    double          digit = 0.0;                  // the digit it appends, 0 to 9
};

/**
 * THE FREE DATE ENTRY (debug.date): any SURA date to jump to, ahead or back. While the keeper set nothing, the
 * fields show the date of the clock record, so after every jump they show the date it landed on - the date
 * of an eclipse ase-ephemeris reported or of a stage the birch reached - and that date can be set again later.
 * A step takes the shown date and moves it; go sends the entry through the date port (ase-calendar computes
 * its game hour, ase-time sets the clock there); now gives the entry back to the clock.
 */
struct VivariumDebugDate {
    std::vector<VivariumDebugDateField> fields;  // debug.date.fields
    std::vector<VivariumDebugDateStep> steps;    // debug.date.steps
    ::godot::Rect2  go_rect;                     // debug.date.go: the jump to the entry
    ::godot::String go_label;
    ::godot::Rect2  now_rect;                    // debug.date.now: the entry back to the clock
    ::godot::String now_label;
    std::vector<VivariumDebugDateKey> keys;      // debug.date.keys: the digits of the typed entry
    ::godot::Rect2  clear_rect;                  // debug.date.clear: the picked part emptied
    ::godot::String clear_label;
    double          digits = 0.0;                // debug.date.digits: digits one part takes at most
    double          base = 0.0;                  // debug.date.base: the base the keys type in, every key digit below it
    uint8_t         typing = GODOT_DATE_PART_NONE;  // GODOT_DATE_PART_* a tap on a field picked; NONE: no field picked
    double          typed = 0.0;                 // digits typed into the picked part since the pick
    bool            held = false;                // the keeper set a date; false: the fields show the clock record
    double          value[GODOT_DATE_PARTS] = {GODOT_DATE_YEAR_FIRST, GODOT_DATE_FIRST, GODOT_DATE_FIRST,
                                               GODOT_DATE_HOUR_FIRST};  // the date held, by GODOT_DATE_PART_*
};

/**
 * One relief button of the debug mode (debug.reliefs of the start configuration): it asks the elevation
 * port for the relief of the whole ground at runtime (GodotHostResourceManager::shape). ase-pl-flora writes
 * it as TER_GEN_HGT_SCL and TER_GEN_BAS_HGT, ase-terrain moves every generated chunk onto the new profile;
 * the seed keeps the shape of the landscape (usecase:6018 derive terrain heights).
 */
struct VivariumDebugRelief {
    ::godot::Rect2  rect;
    ::godot::String label;
    double          height_scale_m = 0.0;  // the height amplitude the button asks, metres
    double          base_height_m = 0.0;   // the ground level the button asks, metres above NN0
};

/**
 * The debug mode: drawn and answered only when the launch names modes.debug.argument. The time lapse and
 * the jump are this host's alone - the requests go through the ports to ase-time of this process and never
 * reach a server clock; a restart boots a host at its default pace.
 *
 * A JUMP SETS THE CLOCK, IT NEVER REPLAYS IT: the seek or the date port moves ase-time's clock to the target
 * in one write, and the frame hands the host only the real time it took (GodotHostResourceManager::advance).
 * The schedules of every module run their passes at the new time while the start screen stands, and every
 * module computes the whole gap since its last state in its own pass; settle_s is the simulated time the
 * view lets them run at the new time before it judges the landing.
 */
struct VivariumDebug {
    bool            on = false;                 // launched with modes.debug.argument
    ::godot::Color  fill;                       // face of a debug button (debug.fill), unlike every game button
    ::godot::Color  text;                       // label and outline of a debug button (debug.text)
    ::godot::Ref<::godot::FontFile> icon_font;  // the Nerd Font of the button icons (debug.icon_font), loaded at run time
    double          icon_web_px = 0.0;          // the font size the offsets of the icon table are meant for (its web_size_px)
    double          settle_s = 0.0;             // simulated seconds that run at the new time after a jump set the clock, before it is judged (debug.settle_s)
    uint32_t        eclipse_hops = 0u;          // how often an eclipse jump searches on from the end of ase-ephemeris' horizon before it notes no_eclipse (debug.eclipse_hops, D2256)
    std::vector<VivariumDebugButton> buttons;   // the time lapse buttons (debug.buttons)
    std::vector<VivariumDebugJump> jumps;       // the jump buttons (debug.jumps)
    std::vector<VivariumDebugRelief> reliefs;   // the relief buttons (debug.reliefs)
    VivariumDebugDate date;                     // the free date entry (debug.date)
    double          height_scale_m = 0.0;       // the height amplitude that stands: the boot's ground.height_scale_m, then the one last granted by the elevation port
    double          factor = GODOT_PACE_NONE;   // the factor asked of the running host, GODOT_PACE_NONE = its default pace
    double          seq = 0.0;                  // number of the last seek or date request of this run, 0 = none taken yet; it only grows, across a rewind too, and starts at ase-time's TIM_CONST_SEEK_SEQ_MIN
    VivariumJump    jump;                       // the jump that runs
};

/**
 * THE LENGTH OF A GAME HOUR AND THE RATE OF THE CLOCK. The hours of a game day and the game seconds of a game
 * hour are ase-time's own constants, carried on the clock record (TIM_CONST_HOURS_PER_DAY and
 * TIM_CONST_SECONDS_PER_HOUR, filed by FloraHostClkExptSystem); the view copies them from every record and
 * never measures them. Measured are only the rate of the clock against the host's simulated real time. The jump
 * computes no target from them: ase-time lays and judges it; the rate is shown, the jump needs none.
 */
struct VivariumHourLength {
    double last_elapsed_s = -1.0;    // game seconds of the clock port in the previous frame, below 0: none yet
    double last_day = -1.0;          // game day of the clock port in the previous frame, below 0: none yet
    double last_hour = -1.0;         // hour of the clock port in the previous frame, below 0: none yet
    double hours_per_day = 0.0;      // game hours of one game day from the clock record, 0: no record yet
    double seconds_per_hour = 0.0;   // game seconds of one game hour from the clock record, 0: no record yet
    double last_simulated_s = -1.0;  // real seconds the host had simulated in the previous frame, below 0: none yet
    double game_per_real = 0.0;      // game seconds the clock runs per simulated real second, 0: not measured yet
};

}  // namespace ase::adp::godot
