#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the constants of the Godot adapter
 * @description Every value the adapter reads that does NOT change at runtime and that is NOT
 *              configuration: the value keys and codes of the host ports of ase-pl-flora, the
 *              table of contents of the staged bundle, the form of the self-test table, the names
 *              of the boot steps a failure is reported under, the GDExtension class binding of
 *              AseVivariumView (class names, the engine callbacks it takes over and their
 *              signature hashes), and its drawing style (colours, font sizes, line widths).
 *
 *              WHAT IS NOT HERE: positions, rectangles, texts, the pour amount and the port and
 *              operation names. They are the client's configuration and come from
 *              res://config/vivarium_start.json (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.4).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @created     2026-10-05
 * @modified    2026-10-07
 * @version     00.00.03.00003
 *
 * WHY THE PORT KEYS ARE WRITTEN A SECOND TIME
 *
 * ase-pl-flora carries the same names (FLORA_HOST_KEY_* in its types.hpp). The adapter sees only
 * the kernel facade - no component, no hub, no unit header (ARCH_ASE_KERNEL_EMBEDDED.md,
 * "Schichten und Grenzen") - so it holds the DOCUMENTED contract of the ports
 * (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3 point 1), exactly as
 * clients/avt-client-android/tests/avt_headless.cpp does. A renamed key breaks the host test of
 * this adapter, which runs against the real bundle.
 *
 * ABBREVIATIONS USED IN THIS ADAPTER:
 *   adp   adapter   third-party isolation module
 *   avt   Antares Vivarium, the client this adapter draws for
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
 * Host ports of ase-pl-flora - value keys (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.3 point 1). The
 * patch port carries one record per patch, its object id is the patch; the clock port one record.
 */
constexpr const char* GODOT_KEY_PLACE = "place";              // patch record: where the view draws the patch
constexpr const char* GODOT_KEY_COVERAGE = "coverage";        // patch record: covered share of the patch, 0..1
constexpr const char* GODOT_KEY_STAGE = "stage";              // patch record: growth stage code (PLT_STAGE)
constexpr const char* GODOT_KEY_CONDITION = "condition";      // patch record: condition code (PLT_CONDITION)
constexpr const char* GODOT_KEY_SOIL_MM = "soil_mm";          // patch record: soil water, mm (HYD_SOIL_WATER)
constexpr const char* GODOT_KEY_SOIL_REL = "soil_rel";        // patch record: soil water over field capacity, 0..1
constexpr const char* GODOT_KEY_CAPACITY_MM = "capacity_mm";  // patch record: field capacity, mm
constexpr const char* GODOT_KEY_AMOUNT_MM = "amount_mm";      // irrigate request: water poured on the patch, mm
constexpr const char* GODOT_KEY_ELAPSED = "elapsed_s";        // clock record: game time, seconds (TIM_ELAPSED_GAME)
constexpr const char* GODOT_KEY_DAY = "day";                  // clock record: game day (TIM_DAY_COUNT)
constexpr const char* GODOT_KEY_HOUR = "hour";                // clock record: hour of the game day (TIM_CURRENT_HOUR)

/**
 * The codes the patch record carries - those of ase-plant (PLANT_STAGE_*, PLANT_COND_*), forwarded
 * by the plugin unchanged. A code outside them is shown as unknown, never mapped onto a known one.
 */
constexpr uint8_t GODOT_STAGE_SEED = 0;       // a seed in the soil
constexpr uint8_t GODOT_STAGE_SPROUT = 1;     // a seedling
constexpr uint8_t GODOT_STAGE_GROW = 2;       // a growing stand
constexpr uint8_t GODOT_STAGE_MATURE = 3;     // a mature stand
constexpr uint8_t GODOT_STAGE_COUNT = 4;      // stages the view has a word and a colour for
constexpr uint8_t GODOT_COND_HEALTHY = 0;     // a stand without a condition mark
constexpr uint8_t GODOT_COND_WILTED = 1;      // a wilted stand
constexpr uint8_t GODOT_COND_DEAD = 2;        // a dead stand
constexpr uint8_t GODOT_COND_COUNT = 3;       // conditions the view has a word for
constexpr uint8_t GODOT_CODE_UNKNOWN = 255u;  // a stage or condition outside the documented codes

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
constexpr const char* GODOT_SELFTEST_KEY_AS = "as";                  // remember: what a later plus refers to
constexpr const char* GODOT_SELFTEST_KEY_PLACE = "place";            // a patch, by its place
constexpr const char* GODOT_SELFTEST_KEY_OBJECT = "object_id";       // irrigate: an object id named directly
constexpr const char* GODOT_SELFTEST_KEY_RECORD = "record";          // "clock" or "patch"
constexpr const char* GODOT_SELFTEST_KEY_KEY = "key";                // the value key of a record
constexpr const char* GODOT_SELFTEST_KEY_OP = "op";                  // eq ne lt le gt ge
constexpr const char* GODOT_SELFTEST_KEY_VALUE = "value";            // the value an op compares with
constexpr const char* GODOT_SELFTEST_KEY_PLUS = "plus";              // a remembered value added to value
constexpr const char* GODOT_SELFTEST_KEY_MAX_FRAMES = "max_frames";  // wait: the bound of a few frames, never a target
constexpr const char* GODOT_SELFTEST_KEY_SECONDS = "seconds";        // catch_up: real seconds handed in at once
constexpr const char* GODOT_SELFTEST_KEY_MAX_STEPS = "max_steps";    // catch_up: most ticks one call may run
constexpr const char* GODOT_SELFTEST_KEY_STEPS = "steps";            // catch_up: ticks in all
constexpr const char* GODOT_SELFTEST_KEY_AMOUNT = "amount_mm";       // irrigate: water poured, mm
constexpr const char* GODOT_SELFTEST_KEY_STATUS = "status";          // irrigate: the expected HostStatus
constexpr const char* GODOT_SELFTEST_KEY_COUNT = "count";            // records: the expected count
constexpr const char* GODOT_SELFTEST_RECORD_CLOCK = "clock";         // record: the clock port
constexpr const char* GODOT_SELFTEST_RECORD_PATCH = "patch";         // record: the patch port
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
 * since the previous one, measured on Godot's monotonic clock - a span the app spent in the
 * background arrives as one long frame and is caught up in ordinary ticks over the next frames.
 */
constexpr double GODOT_CATCH_UP_NOTE_S = 1.0;   // a frame longer than this is a return from the background and gets one line
constexpr uint64_t GODOT_WATER_WAIT_STEPS = 3u;  // Dissemination runs a pour may take to show in the soil water: plugin, hydro, balance, publish

/**
 * Drawing style of AseVivariumView - RGBA, 8 bit per channel (Godot Color::hex).
 */
constexpr uint32_t GODOT_COLOR_BACKGROUND = 0x14181AFFu;    // surface behind everything
constexpr uint32_t GODOT_COLOR_HEADER = 0x1F2A2EFFu;        // band behind title and build line
constexpr uint32_t GODOT_COLOR_SOIL = 0x5B4030FFu;          // bare soil of a patch
constexpr uint32_t GODOT_COLOR_SEED = 0x9C7A44FFu;          // seed dots on bare soil
constexpr uint32_t GODOT_COLOR_SPROUT = 0x8BC34AFFu;        // cover of a seedling
constexpr uint32_t GODOT_COLOR_GROW = 0x4CAF50FFu;          // cover of a growing stand
constexpr uint32_t GODOT_COLOR_MATURE = 0x2E7D32FFu;        // cover of a mature stand
constexpr uint32_t GODOT_COLOR_WILTED = 0xA1887FFFu;        // cover of a wilted stand
constexpr uint32_t GODOT_COLOR_DEAD = 0x6E6552FFu;          // cover of a dead stand
constexpr uint32_t GODOT_COLOR_UNKNOWN = 0x9E9E9EFFu;       // cover of a code the view does not know
constexpr uint32_t GODOT_COLOR_WATER = 0x42A5F5FFu;         // filled part of the soil water bar
constexpr uint32_t GODOT_COLOR_WATER_TRACK = 0x1C2B3AFFu;   // empty part of the soil water bar
constexpr uint32_t GODOT_COLOR_SELECTION = 0xFFD54FFFu;     // frame around the selected patch
constexpr uint32_t GODOT_COLOR_TEXT = 0xECEFF1FFu;          // regular text
constexpr uint32_t GODOT_COLOR_TEXT_DIM = 0x90A4AEFFu;      // secondary text
constexpr uint32_t GODOT_COLOR_BUTTON = 0x37474FFFu;        // enabled button face
constexpr uint32_t GODOT_COLOR_BUTTON_OFF = 0x23292CFFu;    // disabled button face
constexpr uint32_t GODOT_COLOR_ERROR = 0xC62828FFu;         // error panel

constexpr int32_t GODOT_FONT_TITLE = 44;                    // title size (logical pixels)
constexpr int32_t GODOT_FONT_BODY = 28;                     // info lines and button labels
constexpr int32_t GODOT_FONT_SMALL = 20;                    // build line, patch labels, notes
constexpr int32_t GODOT_FONT_MIN = 12;                      // smallest size a line shrinks to so that it fits its width
constexpr float GODOT_FRAME_WIDTH = 6.0f;                   // selection frame line width
constexpr float GODOT_BORDER_WIDTH = 2.0f;                  // patch and button outline width
constexpr float GODOT_PATCH_INSET = 14.0f;                  // gap between patch edge and cover
constexpr float GODOT_BAR_HEIGHT = 16.0f;                   // height of the soil water bar
constexpr float GODOT_LABEL_OFFSET = 34.0f;                 // baseline of a label below a rect's top
constexpr float GODOT_HALF = 0.5f;                          // centre of a span
constexpr float GODOT_LAYOUT_SCALE_MAX = 1.0f;              // a layout is never magnified: spare space is distributed, not filled
constexpr const char* GODOT_FEATURE_MOBILE = "mobile";      // Godot feature of a handheld OS: the window is the screen, its safe area applies
constexpr float GODOT_SEED_DOT_RADIUS = 5.0f;               // radius of one seed dot on bare soil
constexpr int32_t GODOT_SEED_DOTS = 3;                      // seed dots per row and per column
constexpr int32_t GODOT_ERROR_LINES = 6;                    // most lines the error panel wraps to
constexpr int64_t GODOT_LOG_DECIMALS = 3;                   // decimals of a measured value in a log line
constexpr const char* GODOT_LOG_STEP_SCHEDULE = "Dissemination";  // schedule whose runs a log line counts as steps= (the modules' 1 Hz step)
constexpr int64_t GODOT_TEXT_DECIMALS = 1;                  // decimals of a value drawn on the surface
constexpr double GODOT_PERCENT = 100.0;                     // a share drawn as a percentage

}  // namespace ase::adp::godot
