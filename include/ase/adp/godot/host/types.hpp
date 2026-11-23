#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the embedded host of the view: bundle, stage, self-test, measurement and boot
 * @description All compile-time constants of the host sub-type; NO runtime values - those belong in the views.
 *              The staged bundle and the stage of the running build, the self-test table, the measurement mode
 *              and the cycle line, the watch after a ground act, the boot steps, the places in the Godot project,
 *              the GDExtension class and its hooks, the time without a pause, the sowing and the water wait, and
 *              the start screen that covers the initialisation.
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter types.hpp is included by more than a hundred own units of
 *              the avt build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated
 *              by purpose into sub-headers and stays a pure collector of them. These values are read by
 *              GodotHostResourceManager, the registration of the view class and the start screen.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:100364 - host the engine embedded
 * @actor       Entity Registry Steward (ENTIREGSTE)
 * @created     2026-11-22
 * @modified    2026-11-22
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * ABBREVIATIONS: PSS = proportional set size of a process.
 */

/**
 * The staged bundle (clients/avt-client-android/cmake/AvtRuntime.cmake, scripts/stage_native.py):
 * one index file names everything else in the directory - one line per unit, the data root and
 * every data file below it.
 */
constexpr const char* GODOT_BUNDLE_INDEX = "avt_bundle.index";   // table of contents of the bundle
constexpr const char* GODOT_BUNDLE_KEY_DATA_ROOT = "data_root";  // index key: directory of the units' data files
constexpr const char* GODOT_BUNDLE_KEY_UNIT = "unit";            // index key: unit library manifest version api sha256
constexpr const char* GODOT_BUNDLE_KEY_DATA = "data";            // index key: one data file, relative to the data root
constexpr uint32_t GODOT_BUNDLE_UNIT_FIELDS = 6u;                // fields of one unit line
constexpr const char* GODOT_BUNDLE_SEPARATOR = " = ";            // between key and value of an index line
constexpr const char* GODOT_DATA_COPY_ROOT = "user://bundle";    // where the data files are copied when the stage is no directory on disk

/**
 * Where the stage of the running build lies: res://native/<os>-<arch>/<config>/, the layout
 * scripts/stage_native.py writes. os, arch and config are Godot's own words for the running build
 * (OS::get_name lower-cased, Engine::get_architecture_name, OS::is_debug_build) - the same feature
 * words the avt.gdextension sections are keyed by, so the stage the view reads is the stage Godot
 * took the adapter from.
 */
constexpr const char* GODOT_STAGE_ROOT = "res://native";         // root of every staged platform
constexpr const char* GODOT_STAGE_JOIN = "-";                    // between os and arch in a stage directory
constexpr const char* GODOT_STAGE_DEBUG = "debug";               // stage of a debug build
constexpr const char* GODOT_STAGE_RELEASE = "release";           // stage of a release build

/**
 * The self-test table (clients/avt-client-android/config/vivarium_selftest.toml, PLAN_ASE_VIVARIUM_
 * PHASE_02_ANDROID 02.3): the keys of its TOML form, read by GodotHostResourceManager::self_test.
 * Every case checks one step within a few frames; all cases run in order on one test host.
 */
constexpr const char* GODOT_SELFTEST_KEY_TOLERANCE = "tolerance";    // largest |actual - value| of an eq or ne
constexpr const char* GODOT_SELFTEST_KEY_FRAME = "frame_seconds";    // real seconds one host frame stands for
constexpr const char* GODOT_SELFTEST_KEY_RUNS = "runs";              // runs of 00.4 the table covers
constexpr const char* GODOT_SELFTEST_KEY_CASE = "case";              // array of scenarios, in order on the one test host
constexpr const char* GODOT_SELFTEST_KEY_DO = "do";                  // actions of a scenario, in order
constexpr const char* GODOT_SELFTEST_KEY_CHECK = "check";            // run an action checks
constexpr const char* GODOT_SELFTEST_KEY_FRAMES = "frames";          // action: advance n frames
constexpr const char* GODOT_SELFTEST_KEY_REMEMBER = "remember";      // action: keep a value for a later plus
constexpr const char* GODOT_SELFTEST_KEY_IRRIGATE = "irrigate";      // action: submit one pour
constexpr const char* GODOT_SELFTEST_KEY_WAIT = "wait";              // action: advance until a value holds
constexpr const char* GODOT_SELFTEST_KEY_CATCH_UP = "catch_up";      // action: a return from the background
constexpr const char* GODOT_SELFTEST_KEY_EXPECT = "expect";          // action: compare one value
constexpr const char* GODOT_SELFTEST_KEY_RECORDS = "records";        // action: compare a record count
constexpr const char* GODOT_SELFTEST_KEY_IDS = "ids";                // digest: keys carrying an entity id
constexpr const char* GODOT_SELFTEST_KEY_AS = "as";                  // remember: what a later plus refers to
constexpr const char* GODOT_SELFTEST_KEY_PLACE = "place";            // a patch, by its place
constexpr const char* GODOT_SELFTEST_KEY_OBJECT = "object_id";       // irrigate: an object id named directly
constexpr const char* GODOT_SELFTEST_KEY_RECORD = "record";          // "clock" or "patch"
constexpr const char* GODOT_SELFTEST_KEY_KEY = "key";                // the value key of a record
constexpr const char* GODOT_SELFTEST_KEY_OP = "op";                  // eq ne lt le gt ge
constexpr const char* GODOT_SELFTEST_KEY_VALUE = "value";            // the value an op compares with
constexpr const char* GODOT_SELFTEST_KEY_PLUS = "plus";              // a remembered value added to value
constexpr const char* GODOT_SELFTEST_KEY_MAX_FRAMES = "max_frames";  // wait: the bound of a few frames, never a target
constexpr const char* GODOT_SELFTEST_KEY_SECONDS = "seconds";        // catch_up: real seconds the one long frame files at once
constexpr const char* GODOT_SELFTEST_KEY_AMOUNT = "amount_mm";       // irrigate: water poured, mm
constexpr const char* GODOT_SELFTEST_KEY_STATUS = "status";          // irrigate: the expected HostStatus
constexpr const char* GODOT_SELFTEST_KEY_COUNT = "count";            // records: the expected count
constexpr const char* GODOT_SELFTEST_KEY_LAY = "lay";                // action: lay one patch through the scene port
constexpr const char* GODOT_SELFTEST_KEY_SPECIES = "species";        // lay: catalog id text of the species
constexpr const char* GODOT_SELFTEST_KEY_SEED = "seed";              // lay: seed of the plant
constexpr const char* GODOT_SELFTEST_KEY_SOIL = "soil";              // lay: ground word (GODOT_SOIL_WORDS)
constexpr const char* GODOT_SELFTEST_KEY_X = "x";                    // lay: centre of the patch, world metres east
constexpr const char* GODOT_SELFTEST_KEY_Z = "z";                    // lay: centre of the patch, world metres north
constexpr const char* GODOT_SELFTEST_KEY_HALF = "half_m";            // lay: half edge of the patch square, metres
constexpr const char* GODOT_SELFTEST_KEY_CAPACITY = "capacity_mm";   // lay: field capacity of the patch soil, mm
constexpr const char* GODOT_SELFTEST_KEY_GRAVITY = "gravity_rel";    // lay: gravity of the patch relative to g
constexpr const char* GODOT_SELFTEST_KEY_VIEW = "view";              // action: place the eye of the view through the view port
constexpr const char* GODOT_SELFTEST_KEY_Y = "y";                    // view: the eye, world metres up (x east, z north as for lay)
constexpr const char* GODOT_SELFTEST_KEY_AIM_X = "aim_x";            // view: the point the eye looks at, world metres east
constexpr const char* GODOT_SELFTEST_KEY_AIM_Y = "aim_y";            // view: the point the eye looks at, world metres up
constexpr const char* GODOT_SELFTEST_KEY_AIM_Z = "aim_z";            // view: the point the eye looks at, world metres north
constexpr const char* GODOT_SELFTEST_KEY_CAMP = "camp";              // action: lay the camp place through camp_place (PHASE_08 T.0)
constexpr const char* GODOT_SELFTEST_KEY_HULL = "hull";              // action: file every hull corner of one make through camp_hull (T.1)
constexpr const char* GODOT_SELFTEST_KEY_PART = "part";              // action: lay one camp part through camp_part (T.3)
constexpr const char* GODOT_SELFTEST_KEY_DIGEST = "digest";          // action: fold the records of named ports frame by frame (T.10)
constexpr const char* GODOT_SELFTEST_KEY_MAKE = "make";              // hull, part: the make by its name text
constexpr const char* GODOT_SELFTEST_KEY_CORNERS = "corners";        // hull: array of [hx, hy, hz] corners, metres from the centre of mass
constexpr const char* GODOT_SELFTEST_KEY_DROP = "drop";              // part: release height above the stem foot, metres
constexpr const char* GODOT_SELFTEST_KEY_FALL = "fall";              // part: fall speed in still air, m/s
constexpr const char* GODOT_SELFTEST_KEY_MASS = "mass";              // part: mass of the make, kg
constexpr const char* GODOT_SELFTEST_KEY_FUEL = "fuel";              // part: true on a part that burns
constexpr const char* GODOT_SELFTEST_PORT_FIRE = "survival_fire";    // digest: a port by its name in the start file, the fire cells
constexpr const char* GODOT_SELFTEST_PORT_CAMP_BODY = "camp_body";   // digest: the place and turn of every camp part body
constexpr uint32_t GODOT_SELFTEST_HULL_AXES = 3u;                    // hull: hx, hy and hz of one corner
constexpr uint64_t GODOT_WALK_DIGEST_BASIS = 1469598103934665603ull; // digest: FNV-1a 64 offset basis, as the Linux reader
constexpr uint64_t GODOT_WALK_DIGEST_PRIME = 1099511628211ull;       // digest: FNV-1a 64 prime
constexpr const char* GODOT_SELFTEST_RECORD_CLOCK = "clock";         // record: the clock port
constexpr const char* GODOT_SELFTEST_RECORD_PATCH = "patch";         // record: the patch port; an organ port by its GODOT_ORGAN_WORDS
constexpr const char* GODOT_SELFTEST_RECORD_SOIL = "soil";           // record: the soil port, one record per patch with its place (usecase:101340)
constexpr const char* GODOT_SELFTEST_OP_EQ = "eq";                   // |actual - value| <= tolerance
constexpr const char* GODOT_SELFTEST_OP_NE = "ne";                   // |actual - value| > tolerance
constexpr const char* GODOT_SELFTEST_OP_LT = "lt";                   // actual < value
constexpr const char* GODOT_SELFTEST_OP_LE = "le";                   // actual <= value
constexpr const char* GODOT_SELFTEST_OP_GT = "gt";                   // actual > value
constexpr const char* GODOT_SELFTEST_OP_GE = "ge";                   // actual >= value
constexpr uint32_t GODOT_SELFTEST_DECIMALS = 4u;                     // decimals of actual and expected in a report line
constexpr uint32_t GODOT_NUMBER_BYTES = 32u;                         // text of one number in a report line, NUL included
constexpr int GODOT_LOG_LEVEL_MAX = 5;                               // highest level a host line carries (spdlog critical)

/**
 * Measurement mode (PLAN 02.3, acceptance A10): percentiles of the host time (advance + snapshot)
 * and of the whole _process interval, both in microseconds, and the PSS of the process.
 */
constexpr uint32_t GODOT_MEASURE_P50 = 50u;                          // median
constexpr uint32_t GODOT_MEASURE_P95 = 95u;                          // budget percentile of the plan
constexpr uint32_t GODOT_MEASURE_P99 = 99u;                          // tail
constexpr uint32_t GODOT_MEASURE_PERCENT = 100u;                     // whole of a percentile
constexpr double GODOT_MEASURE_FRAMES_RESERVED = 240.0;              // frames per second the buffers are reserved for
constexpr double GODOT_USEC_PER_SECOND = 1000000.0;                  // Time::get_ticks_usec per second
constexpr const char* GODOT_PSS_SOURCE = "/proc/self/smaps_rollup";  // proportional set size of this process
constexpr const char* GODOT_PSS_KEY = "Pss:";                         // its line, value in kB

/**
 * Measurement line of one cycle (PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.6, godot_cycle_measure.hpp):
 * the organs of the organ ports per branching order, one count per order up to this many; an organ of
 * a deeper order counts in the last one, which the line names as "<n>+".
 */
constexpr uint32_t GODOT_MEASURE_ORDERS = 16u;                       // orders counted one by one

/**
 * The frames after one ground act (lower, raise, flatten): the view watches the _process intervals of this
 * many frames after the act and writes one line with the longest of them against the budget of a frame that
 * carries a changed ground (P0689: the terrain travels as a delta, no frame of a stab takes longer).
 */
constexpr uint32_t GODOT_GROUND_WATCH_FRAMES = 30u;                  // frames watched after one ground act
constexpr uint64_t GODOT_GROUND_WATCH_BUDGET_USEC = 100000u;         // longest interval one of them may take

/**
 * Boot steps of GodotHostResourceManager::boot - the name a failure is reported under.
 */
constexpr const char* GODOT_STEP_NONE = "";                // no failure recorded
constexpr const char* GODOT_STEP_CONFIG = "start_config";  // the start configuration is incomplete
constexpr const char* GODOT_STEP_CREATE = "create";        // KernelEmbeddedHost::create
constexpr const char* GODOT_STEP_BUNDLE = "bundle_index";  // reading the bundle index
constexpr const char* GODOT_STEP_MANIFEST = "manifest";    // reading a staged manifest
constexpr const char* GODOT_STEP_DATA_ROOT = "data_root";  // KernelEmbeddedHost::set_data_root
constexpr const char* GODOT_STEP_LOAD = "load_units";      // KernelEmbeddedHost::load_units
constexpr const char* GODOT_STEP_PORT = "port";            // a configured port is not offered
constexpr const char* GODOT_STEP_START = "start";          // KernelEmbeddedHost::start
constexpr const char* GODOT_STEP_ELEVATION = "elevation";  // the shape request of the configured relief was refused
constexpr const char* GODOT_STEP_LAY = "lay";              // the lay request of a configured patch was refused
constexpr const char* GODOT_STEP_GROUND = "ground";        // the lay request of the configured ground square was refused
constexpr const char* GODOT_STEP_BROOK = "brook";          // the lay request of a configured brook was refused
constexpr const char* GODOT_STEP_ADVANCE = "advance";      // KernelEmbeddedHost::advance
constexpr const char* GODOT_STEP_SNAPSHOT = "snapshot";    // reading a port snapshot
constexpr const char* GODOT_STEP_STOP = "stop";            // KernelEmbeddedHost::stop
constexpr const char* GODOT_LOG_SOURCE = "AVT";            // tag of the adapter's own log lines

/**
 * Places in the Godot project and its user directory.
 */
constexpr const char* GODOT_CONFIG_PATH = "res://config/vivarium_start.json";  // start configuration staged by avt-stage
constexpr const char* GODOT_LOG_PATH = "user://logs/ase-embedded.log";         // ASE log file, resolved to an absolute path
constexpr const char* GODOT_IDENTITY_FILE = "build.identity";                  // stage identity beside the libraries
constexpr const char* GODOT_TEXT_CONFIG_ERROR = "Erro de configuração";        // panel title while the configuration itself is unreadable
constexpr int64_t GODOT_RECT_FIELDS = 4;      // x, y, width, height of a rectangle in the configuration
constexpr int64_t GODOT_POINT_FIELDS = 2;     // x, y of a text anchor in the configuration
constexpr uint32_t GODOT_NO_OBJECT = UINT32_MAX;  // object id that means: nothing selected, nothing pending (a patch entity may be 0)
constexpr uint32_t GODOT_NO_PLACE = UINT32_MAX;   // place that means: the record names none

/**
 * The GDExtension class AseVivariumView (godot_registration.cpp): its name, the engine class its
 * objects are built from, and the engine callbacks the view takes over. Godot asks for each
 * callback by name AND by the hash of its signature; the hashes are those of the pinned
 * extension API (godot-cpp e83fd0904c = godot-4.5-stable, gdextension/extension_api.json). A
 * different hash is a different signature and is answered with "not taken over".
 */
constexpr const char* GODOT_VIEW_CLASS = "AseVivariumView";       // class the scene instantiates
constexpr const char* GODOT_VIEW_ENGINE_CLASS = "Node2D";         // engine class of the view's object
constexpr const char* GODOT_HOOK_READY = "_ready";                // Node: in the tree, children ready
constexpr uint32_t GODOT_HOOK_READY_HASH = 3218959716u;           // signature void ()
constexpr const char* GODOT_HOOK_PROCESS = "_process";            // Node: once per frame
constexpr uint32_t GODOT_HOOK_PROCESS_HASH = 373806689u;          // signature void (float delta)
constexpr const char* GODOT_HOOK_DRAW = "_draw";                  // CanvasItem: redraw requested
constexpr uint32_t GODOT_HOOK_DRAW_HASH = 3218959716u;            // signature void ()
constexpr const char* GODOT_HOOK_INPUT = "_unhandled_input";      // Node: input no control consumed
constexpr uint32_t GODOT_HOOK_INPUT_HASH = 3754044979u;           // signature void (InputEvent event)
constexpr const char* GODOT_HOOK_EXIT = "_exit_tree";             // Node: leaving the tree
constexpr uint32_t GODOT_HOOK_EXIT_HASH = 3218959716u;            // signature void ()

/**
 * Time without a pause (PLAN 01.3 point 4): every frame hands the host the real time that passed
 * since the previous one, measured on Godot's monotonic clock, and files the absolute real
 * position on the real port. A span the app spent in the background arrives as one long frame:
 * ase-time sets its clock forward over it in one write and reports the position it laid
 * (TIM_REAL_LAID_S); nothing is replayed in ticks.
 */
constexpr double GODOT_CATCH_UP_NOTE_S = 1.0;   // a frame longer than this is a return from the background and gets one line
constexpr uint32_t GODOT_SOWING_FIRST = 0u;      // the first sowing of a view: the seeds of the start configuration (godot_new_seed.hpp)
constexpr uint64_t GODOT_WATER_WAIT_STEPS = 3u;  // Dissemination runs a pour may take to show in the soil water: plugin, hydro, balance, publish
constexpr double GODOT_WATER_ARRIVED_SHARE = 0.5;  // share of the expected rise (the pour, or the room up to field capacity if smaller) the soil water must gain before the pour counts as arrived; drift between press and arrival moves it by thousandths of a millimetre

/**
 * The start screen of AseVivariumView: it covers the initialisation and lifts when the first frame shows everything.
 */
constexpr uint32_t GODOT_INIT_RENDER_FRAMES = 10u;          // frames in a row the complete scene renders under the start screen without a new pipeline compilation before it lifts
constexpr uint32_t GODOT_INIT_SCENE_PARTS = 5u;             // parts the scene must stand with: terrain, streams, templates, organs, camera focus
constexpr double GODOT_INIT_STEPS = 6.0;                    // equal steps of the progress bar: patches, brooks, scene parts, sky and clock, figure file, quiet frames
constexpr float GODOT_PROGRESS_WIDTH_SHARE = 0.6f;          // width of the progress bar as share of the place area
constexpr float GODOT_PROGRESS_HEIGHT = 14.0f;              // height of the progress bar (logical pixels)
constexpr float GODOT_PROGRESS_GAP = 36.0f;                 // gap between the loading line and the progress bar, and the bar and its percent line
constexpr uint32_t GODOT_SPINNER_DOTS = 8u;                 // dots of the spinner above the loading line
constexpr float GODOT_SPINNER_RADIUS = 34.0f;               // radius of the spinner circle (logical pixels)
constexpr float GODOT_SPINNER_DOT = 6.0f;                   // radius of one spinner dot (logical pixels)
constexpr float GODOT_SPINNER_GAP = 70.0f;                  // gap between the spinner centre and the loading line
constexpr uint64_t GODOT_SPINNER_PERIOD_MS = 1200u;         // milliseconds of one spinner turn
constexpr float GODOT_SPINNER_DIM = 0.2f;                   // opacity of the dot farthest behind the leading one

}  // namespace ase::adp::godot
