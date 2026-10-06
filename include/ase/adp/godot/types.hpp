#pragma once

/**
 * =============================================================================
 * ASE MODULE TYPES (SSOT)
 * =============================================================================
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the constants of the Godot adapter
 * @description Every value the adapter reads that does NOT change at runtime and that is NOT
 *              configuration: the key names of the host port vegetation.patch.v1, the table of
 *              contents of the staged bundle, the names of the boot steps a failure is reported
 *              under, the GDExtension class binding of AseVivariumView (class names, the engine
 *              callbacks it takes over and their signature hashes), and its drawing style
 *              (colours, font sizes, line widths).
 *
 *              WHAT IS NOT HERE: positions, rectangles, texts, start values and the port and
 *              operation names. They are the client's configuration and come from
 *              res://config/vivarium_start.json (PLAN_ASE_VIVARIUM_PHASE_01_INTEG 01.2: "Die
 *              Konfiguration wird vom Client als Daten geladen").
 *
 * -----------------------------------------------------------------------------
 * META
 * -----------------------------------------------------------------------------
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    ecs/module
 * @created     2026-10-05
 * @modified    2026-10-05
 * @version     00.00.01.00001
 *
 * -----------------------------------------------------------------------------
 * WHY THE PORT KEYS ARE WRITTEN A SECOND TIME
 * -----------------------------------------------------------------------------
 * The vegetation plugin carries the same names (VEG_HOST_KEY_* in its types.hpp). The adapter
 * may not include a plugin header (PLAN 01.2: "weder ase::pl-vegetation noch Plugin-Header"),
 * so it holds the DOCUMENTED contract of the port (PLAN_ASE_VIVARIUM_PHASE_00_CONTRACT 00.3),
 * exactly as clients/avt-client-android/tests/avt_headless.cpp does. A renamed key breaks the
 * host test of this adapter, which runs against the real plugin library.
 *
 * ABBREVIATIONS USED IN THIS ADAPTER:
 *   adp   adapter   third-party isolation module
 *   avt   Antares Vivarium, the client this adapter draws for
 *
 * -----------------------------------------------------------------------------
 * ECS TYPES COMPLIANCE
 * -----------------------------------------------------------------------------
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 *
 * =============================================================================
 */

#include <cstdint>

namespace ase::adp::godot {

/**
 * Host port vegetation.patch.v1 - value keys (PLAN_ASE_VIVARIUM_PHASE_00_CONTRACT 00.3).
 */
constexpr const char* GODOT_KEY_BIOMASS = "biomass";        // configure + snapshot: patch cover
constexpr const char* GODOT_KEY_MOISTURE = "moisture";      // configure + snapshot: water reserve
constexpr const char* GODOT_KEY_AGE = "age_seconds";        // configure + snapshot: patch age
constexpr const char* GODOT_KEY_AMOUNT = "amount";          // submit irrigate: water to add
constexpr const char* GODOT_KEY_SEED = "seed";              // snapshot: 1 while sown
constexpr const char* GODOT_KEY_SPROUT = "sprout";          // snapshot: 1 while sprouting
constexpr const char* GODOT_KEY_MATURE = "mature";          // snapshot: 1 at full cover
constexpr const char* GODOT_KEY_DEAD = "dead";              // snapshot: 1 once died back
constexpr double GODOT_SNAPSHOT_FLAG_SET = 0.5;             // a stage value at or above this is set (port sends 0 or 1)

/**
 * The staged bundle (clients/avt-client-android/cmake/AvtRuntime.cmake, scripts/stage_native.py):
 * one index file names everything else in the directory.
 */
constexpr const char* GODOT_BUNDLE_INDEX = "avt_bundle.index";  // table of contents of the bundle
constexpr const char* GODOT_BUNDLE_KEY_LIBRARY = "library";     // index key: plugin library file
constexpr const char* GODOT_BUNDLE_KEY_MANIFEST = "manifest";   // index key: staged manifest file
constexpr const char* GODOT_BUNDLE_KEY_PLUGIN = "plugin";       // index key: plugin name
constexpr const char* GODOT_BUNDLE_KEY_VERSION = "version";     // index key: plugin version
constexpr const char* GODOT_BUNDLE_KEY_API = "api_version";     // index key: ASE plugin API

/**
 * Boot steps of GodotHostResourceManager::boot - the name a failure is reported under.
 */
constexpr const char* GODOT_STEP_NONE = "";                // no failure recorded
constexpr const char* GODOT_STEP_CONFIG = "start_config";  // the start configuration is incomplete
constexpr const char* GODOT_STEP_CREATE = "create";        // KernelEmbeddedHost::create
constexpr const char* GODOT_STEP_BUNDLE = "bundle_index";  // reading the bundle index
constexpr const char* GODOT_STEP_MANIFEST = "manifest";    // reading the staged manifest
constexpr const char* GODOT_STEP_LOAD = "load_plugin";     // KernelEmbeddedHost::load_plugin
constexpr const char* GODOT_STEP_PORT = "port";            // the configured port is not offered
constexpr const char* GODOT_STEP_CONFIGURE = "configure";  // creating one start patch
constexpr const char* GODOT_STEP_START = "start";          // KernelEmbeddedHost::start
constexpr const char* GODOT_STEP_TICK = "tick";            // KernelEmbeddedHost::tick
constexpr const char* GODOT_STEP_SNAPSHOT = "snapshot";    // reading the port snapshot
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
constexpr uint32_t GODOT_NO_OBJECT = 0u;      // object id that means: nothing selected, nothing pending

/**
 * The GDExtension class AseVivariumView (godot_registration.cpp): its name, the engine class its
 * objects are built from, and the engine callbacks the view takes over. Godot asks for each
 * callback by name AND by the hash of its signature; the hashes are those of the pinned
 * extension API (godot-cpp e83fd0904c = godot-4.5-stable, gdextension/extension_api.json). A
 * different hash is a different signature and is answered with "not taken over".
 */
constexpr const char* GODOT_VIEW_CLASS = "AseVivariumView";       // class name the scene instantiates
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
 * Stage of a patch as the view draws it, derived from the port's four flags (dead first).
 */
constexpr uint8_t GODOT_STAGE_NONE = 0;       // no flag set
constexpr uint8_t GODOT_STAGE_SEED = 1;       // sown, nothing above ground
constexpr uint8_t GODOT_STAGE_SPROUT = 2;     // growing
constexpr uint8_t GODOT_STAGE_MATURE = 3;     // full cover
constexpr uint8_t GODOT_STAGE_DEAD = 4;       // died back

/**
 * Drawing style of AseVivariumView - RGBA, 8 bit per channel (Godot Color::hex).
 */
constexpr uint32_t GODOT_COLOR_BACKGROUND = 0x14181AFFu;    // surface behind everything
constexpr uint32_t GODOT_COLOR_HEADER = 0x1F2A2EFFu;        // band behind title and build line
constexpr uint32_t GODOT_COLOR_SOIL = 0x5B4030FFu;          // bare soil of a patch
constexpr uint32_t GODOT_COLOR_SEED = 0x9C7A44FFu;          // cover of a sown patch
constexpr uint32_t GODOT_COLOR_SPROUT = 0x8BC34AFFu;        // cover of a sprouting patch
constexpr uint32_t GODOT_COLOR_MATURE = 0x2E7D32FFu;        // cover of a mature patch
constexpr uint32_t GODOT_COLOR_DEAD = 0x6E6552FFu;          // cover of a died-back patch
constexpr uint32_t GODOT_COLOR_MOISTURE = 0x42A5F5FFu;      // filled part of the moisture bar
constexpr uint32_t GODOT_COLOR_MOISTURE_TRACK = 0x1C2B3AFFu; // empty part of the moisture bar
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
constexpr float GODOT_BAR_HEIGHT = 16.0f;                   // height of the moisture bar
constexpr float GODOT_LABEL_OFFSET = 34.0f;                 // baseline of a label below a rect's top
constexpr float GODOT_HALF = 0.5f;                          // centre of a span
constexpr float GODOT_SEED_DOT_RADIUS = 5.0f;               // radius of one seed dot on bare soil
constexpr int32_t GODOT_SEED_DOTS = 3;                      // seed dots per row and per column
constexpr int32_t GODOT_ERROR_LINES = 6;                    // most lines the error panel wraps to
constexpr int64_t GODOT_LOG_DECIMALS = 3;                   // decimals of a measured value in a log line
constexpr int64_t GODOT_TEXT_DECIMALS = 1;                  // decimals of a value drawn on the surface

}  // namespace ase::adp::godot
