#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for the drawing style, layout and HUD words of the care view
 * @description All compile-time constants of the hud sub-type; NO runtime values - those belong in the views.
 *              The colours, fonts and layout of AseVivariumView, the easing and the ring of the HUD, the path
 *              reader of a carried thing, the backend gaps with their register entries, the debug tabs, the hand
 *              modes and acts, the sign anchors, the weather kinds, the touch acts and the words of drag and drop.
 *
 *              WHY A TYPES HEADER OF ITS OWN: the adapter types.hpp is included by more than a hundred own units of
 *              the avt build and takes no new line (header ratchet, operator decision 2026-11-21); it is dedicated
 *              by purpose into sub-headers and stays a pure collector of them. These values are read by the HUD,
 *              its bridge and the drawing of AseVivariumView.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
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
 * ABBREVIATIONS: HUD = head-up display, the interface layer over the scene.
 */

/**
 * Drawing style of AseVivariumView - RGBA, 8 bit per channel (Godot Color::hex).
 */
constexpr uint32_t GODOT_COLOR_BACKGROUND = 0x14181AFFu;    // surface behind everything
constexpr uint32_t GODOT_COLOR_PROGRESS = 0x8BC34AFFu;      // filled part of the start screen progress bar
constexpr uint32_t GODOT_COLOR_PROGRESS_TRACK = 0x37474FFFu;  // empty part of the start screen progress bar
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
constexpr float GODOT_BORDER_WIDTH_ACTIVE = 4.0f;           // outline of the debug button whose time lapse is running
constexpr float GODOT_PATCH_INSET = 14.0f;                  // gap between patch edge and cover
constexpr float GODOT_BAR_HEIGHT = 16.0f;                   // height of the soil water bar
constexpr float GODOT_LABEL_OFFSET = 34.0f;                 // baseline of a label below a rect's top
constexpr float GODOT_HALF = 0.5f;                          // centre of a span
constexpr float GODOT_LAYOUT_SCALE_MAX = 1.0f;              // a layout is never magnified: spare space is distributed, not filled
constexpr const char* GODOT_FEATURE_MOBILE = "mobile";      // Godot feature of a handheld OS: the window is the screen, its safe area applies
constexpr const char* GODOT_DISPLAY_HEADLESS = "headless";  // name of the display server that opens no window (usecase:101019 start the client window)
constexpr uint32_t GODOT_WINDOW_NUMBER_CHARS = 32u;         // text room of one measured value in the window line
constexpr uint32_t GODOT_WINDOW_DECIMALS = 2u;              // decimals of a surface size or screen scale in the window line
constexpr float GODOT_SEED_DOT_RADIUS = 5.0f;               // radius of one seed dot on bare soil
constexpr int32_t GODOT_SEED_DOTS = 3;                      // seed dots per row and per column
constexpr int32_t GODOT_ERROR_LINES = 6;                    // most lines the error panel wraps to
constexpr int64_t GODOT_LOG_DECIMALS = 3;                   // decimals of a measured value in a log line
constexpr const char* GODOT_LOG_STEP_SCHEDULE = "Dissemination";  // schedule whose runs a log line counts as steps= (the modules' 1 Hz step)
constexpr const char* GODOT_DAY_TURN_SCHEDULE = "GameDawn";        // game-time schedule ase-calendar requests once per crossed TUMBRAE; its runs close a measured cycle
constexpr int64_t GODOT_TEXT_DECIMALS = 1;                  // decimals of a value drawn on the surface
constexpr double GODOT_PERCENT = 100.0;                     // a share drawn as a percentage

// The motion of the HUD (usecase:101440, 06.3 UiMotion): a design token names a CSS easing, these are the
// control points CSS Easing Functions Level 1 defines for its keywords, and the solver of cubic-bezier().
constexpr const char* GODOT_EASE_LINEAR = "linear";                 // CSS keyword: progress equals time
constexpr const char* GODOT_EASE = "ease";                          // CSS keyword ease
constexpr const char* GODOT_EASE_IN = "ease-in";                    // CSS keyword ease-in
constexpr const char* GODOT_EASE_OUT = "ease-out";                  // CSS keyword ease-out
constexpr const char* GODOT_EASE_IN_OUT = "ease-in-out";            // CSS keyword ease-in-out
constexpr const char* GODOT_EASE_BEZIER = "cubic-bezier(";          // head of a CSS cubic-bezier(x1, y1, x2, y2)
constexpr const char* GODOT_EASE_BEZIER_END = ")";                  // tail of that function
constexpr const char* GODOT_EASE_BEZIER_SEPARATOR = ",";            // separator of its four numbers
constexpr int64_t GODOT_EASE_BEZIER_POINTS = 4;                     // x1, y1, x2, y2: the numbers a cubic-bezier() carries
constexpr double GODOT_EASE_DEFAULT_X1 = 0.25;                      // ease = cubic-bezier(0.25, 0.1, 0.25, 1): first control x
constexpr double GODOT_EASE_DEFAULT_Y1 = 0.1;                       // ease: first control y (CSS Easing Level 1)
constexpr double GODOT_EASE_DEFAULT_X2 = 0.25;                      // ease: second control x (CSS Easing Level 1)
constexpr double GODOT_EASE_DEFAULT_Y2 = 1.0;                       // ease: second control y (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_X1 = 0.42;                           // ease-in = cubic-bezier(0.42, 0, 1, 1): first control x
constexpr double GODOT_EASE_IN_Y1 = 0.0;                            // ease-in: first control y (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_X2 = 1.0;                            // ease-in: second control x (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_Y2 = 1.0;                            // ease-in: second control y (CSS Easing Level 1)
constexpr double GODOT_EASE_OUT_X1 = 0.0;                           // ease-out = cubic-bezier(0, 0, 0.58, 1): first control x
constexpr double GODOT_EASE_OUT_Y1 = 0.0;                           // ease-out: first control y (CSS Easing Level 1)
constexpr double GODOT_EASE_OUT_X2 = 0.58;                          // ease-out: second control x (CSS Easing Level 1)
constexpr double GODOT_EASE_OUT_Y2 = 1.0;                           // ease-out: second control y (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_OUT_X1 = 0.42;                       // ease-in-out = cubic-bezier(0.42, 0, 0.58, 1): first control x
constexpr double GODOT_EASE_IN_OUT_Y1 = 0.0;                        // ease-in-out: first control y (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_OUT_X2 = 0.58;                       // ease-in-out: second control x (CSS Easing Level 1)
constexpr double GODOT_EASE_IN_OUT_Y2 = 1.0;                        // ease-in-out: second control y (CSS Easing Level 1)
constexpr double GODOT_EASE_LINEAR_X1 = 0.0;                        // linear as a bezier on the diagonal: first control x
constexpr double GODOT_EASE_LINEAR_Y1 = 0.0;                        // linear on the diagonal: first control y
constexpr double GODOT_EASE_LINEAR_X2 = 1.0;                        // linear on the diagonal: second control x
constexpr double GODOT_EASE_LINEAR_Y2 = 1.0;                        // linear on the diagonal: second control y
constexpr int32_t GODOT_EASE_SOLVE_STEPS = 24;                      // bisection steps that find the curve parameter of a time (2^-24 of the span)
constexpr double GODOT_EASE_SOLVE_EPSILON = 1.0e-6;                 // time distance (share of the duration) at which the parameter counts as found

// The ring of the HUD (usecase:101440, 06.3 UiRing): the mockup's arcP measures an angle in degrees from the
// top clockwise, closes a full turn just short of 360 so that the path stays an arc, and drops a sweep below
// a tenth of a degree.
constexpr double GODOT_RING_TOP_DEG = 90.0;                         // arcP: (a - 90) degrees turns the mockup's top to Godot's angle 0 at +x
constexpr double GODOT_RING_FULL_DEG = 359.9;                       // arcP: degrees a full-turn sweep is cut to, so the path stays one arc
constexpr double GODOT_RING_MIN_DEG = 0.1;                          // ARC: degrees below which a sweep draws no path
constexpr double GODOT_RING_TURN_DEG = 360.0;                       // degrees of one turn
constexpr double GODOT_RING_POINTS_PER_TURN = 96.0;                 // polyline points draw_arc gets for a full circle; a shorter arc gets its share
constexpr int32_t GODOT_RING_POINTS_MIN = 3;                        // fewest polyline points of the shortest arc
constexpr double GODOT_TEXT_ANCHOR_START = 0.0;                     // text-anchor start: the whole width right of x
constexpr double GODOT_TEXT_ANCHOR_MIDDLE = 0.5;                    // text-anchor middle: half the width left of x
constexpr double GODOT_TEXT_ANCHOR_END = 1.0;                       // text-anchor end: the whole width left of x
constexpr int64_t GODOT_TEXT_RASTER_MIN = 1;                        // smallest whole raster size a HUD line is shaped at
constexpr int32_t GODOT_HEX_CORNERS = 6;                            // corners of a HUD hexagon chip, the first at 0 degrees

// The drawing of a carried thing (usecase:101440, 06.3 UiItemSprite): item_art.json writes each path as SVG
// path data; the reader knows the commands generate-item-art.cjs lets through and flattens every curve.
constexpr const char* GODOT_ITEM_PATH_COMMANDS = "MLHVCQAZmlhvcqaz";  // the path commands of item_art.json (generate-item-art.cjs PATH_COMMANDS)
constexpr const char* GODOT_ITEM_PATH_SEPARATORS = " ,\t\r\n";       // characters between two numbers of SVG path data
constexpr int32_t GODOT_ITEM_CURVE_POINTS = 16;                       // points one Bezier piece or one arc is flattened to
constexpr int32_t GODOT_ITEM_POLYGON_MIN = 3;                         // fewest corners a filled outline needs
constexpr double GODOT_ITEM_POINT_EPSILON = 1.0e-4;                   // view-box distance at which two outline points count as one

// The backend gaps of the care view (PLAN_ASE_VIVARIUM_PHASE_06_HUD bridge section, carrier N0448): while a port
// does not deliver a value, the bridge writes the mockup's value from SHA into the frame and the picture stays
// whole; VivariumView::open_gap logs the gap once on its edge with its register entry, never in the picture.
// The register is the one list; these name its entries.
constexpr const char* GODOT_HUD_GAP_CARRY = "N0451";                  // flora.carry.v1: things, place, containers, load (ase-inventory)
constexpr const char* GODOT_HUD_GAP_STOW = "N0452";                   // flora.stow.v1: moving a thing with its refusal code
constexpr const char* GODOT_HUD_GAP_WEATHER = "N0449";                // sky.weather.v1: the weather kind, wind and the forecast (ase-atmosphere, ase-pl-sky)
constexpr const char* GODOT_HUD_GAP_PATCH = "N0454";                  // flora.patch.v1: soil water and the preview after watering
constexpr const char* GODOT_HUD_GAP_LOOSE = "N0456";                  // the loosened share of the soil after the spade (loosen the soil)
constexpr const char* GODOT_HUD_GAP_SAP = "N0457";                    // birch sap: a running tap and the fill of its vessel
constexpr const char* GODOT_HUD_GAP_PACE = "N0459";                   // flora.pace.v1: the factor ase-time runs this host's clock at
constexpr const char* GODOT_HUD_GAP_ROOT = "N0463";                   // flora.organ.root.v1: the root segments of the patch's body under the trunk
constexpr const char* GODOT_HUD_GAP_CLOCK = "N0460";                  // flora.clock.v1: season, light, phase starts, shares of the day and the year (ase-calendar, ase-time)
constexpr const char* GODOT_HUD_GAP_TRAIL = "N0450";                  // flora.trail.v1: the past events with their share of the year and domain
constexpr const char* GODOT_HUD_GAP_SIGN = "N0461";                   // the state fields of each sign case on the patch set (wilt, dead, flush, shed, rest, catkin, sap, brook)
constexpr const char* GODOT_HUD_GAP_PRUNE = "N0462";                  // flora.scene.v1: the internodes that may be cut

// The debug layer (usecase:101120, 06.4 DebugLayer, the mockup's debugLayer()): its four tabs in the order of
// texts.hud.debug_tabs, and the index no chip has.
constexpr uint32_t GODOT_HUD_DEBUG_TAB_TEMPO = 0u;                    // the time lapse: debug.buttons, flora.pace.v1
constexpr uint32_t GODOT_HUD_DEBUG_TAB_SALTO = 1u;                    // the jumps: debug.jumps, flora.seek.v1 and flora.date.v1
constexpr uint32_t GODOT_HUD_DEBUG_TAB_DATA = 2u;                     // the free date entry: debug.date, flora.date.v1
constexpr uint32_t GODOT_HUD_DEBUG_TAB_RELEVO = 3u;                   // the relief: debug.reliefs, flora.elevation.v1
constexpr uint32_t GODOT_HUD_DEBUG_TABS = 4u;                         // tabs of the drawer
constexpr uint32_t GODOT_HUD_DEBUG_NONE = UINT32_MAX;                 // a hit on no chip of that kind

// The hand slot (usecase:100367, 06.4 HandSlot, the mockup's hand()): the modes whose lines it speaks, named as
// hud.modes, the ops of a terrain verb (the token hud_verb_<v>_op), the hold word of a tool at work
// (hud_item_<id>_hold) and the acts of the hand, the stow button and a verb chip.
constexpr const char* GODOT_HUD_MODE_REST = "rest";                   // nothing chosen: the empty hand may say where to start
constexpr const char* GODOT_HUD_MODE_WATER = "water";                 // the can: pour, its arcs, the soil before and after
constexpr const char* GODOT_HUD_MODE_PRUNE = "prune";                 // the pruning knife: the shoots left to cut
constexpr const char* GODOT_HUD_MODE_SOW = "sow";                     // the planter: the seeds left in the pouch
constexpr const char* GODOT_HUD_MODE_TERRAIN = "terrain";             // a ground tool: its verbs above the hand
constexpr const char* GODOT_HUD_MODE_SAP = "sap";                     // the auger: the spouts left for a tap
constexpr const char* GODOT_HUD_MODE_PLUG = "plug";                   // the peg: the sap the vessel holds
constexpr const char* GODOT_HUD_VERB_OP_BED = "bed";                  // lays a brook bed between two points
constexpr const char* GODOT_HUD_VERB_OP_LEVEL = "level";              // levels the whole area
constexpr const char* GODOT_HUD_VERB_OP_MEASURE = "measure";          // measures one point
constexpr const char* GODOT_HUD_VERB_TOKEN = "hud_verb_";            // theme: the head of every verb token hud_verb_<v>_*
constexpr const char* GODOT_HUD_VERB_OP_TAIL = "_op";                 // theme: hud_verb_<v>_op, the terrain op of the verb
constexpr const char* GODOT_HUD_VERB_TONE_TAIL = "_tone";             // theme: hud_verb_<v>_tone, the domain its hexagon draws in
constexpr const char* GODOT_HUD_VERB_BRUSH_TAIL = "_brush_m";         // theme: hud_verb_<v>_brush_m, the radius a brush verb works under, metres
constexpr int64_t GODOT_HUD_GROUND_NO_SAMPLE = -1;                    // UiGround: a lattice point the terrain port carried no sample for
constexpr const char* GODOT_HUD_ITEM_HOLD_NONE = "none";              // a tool at work: the hand offers its use
constexpr const char* GODOT_HUD_ACT_POUR = "pour";                    // the can in the hand in the mode water: pours while held
constexpr const char* GODOT_HUD_ACT_STOW = "drop";                    // the stow button: the thing goes back where it came from
constexpr const char* GODOT_HUD_ACT_VERB = "terr";                    // a verb chip; its verb goes with it

// The anchors a sign stands at (usecase:100370, 06.4 SignMarker, the token hud_sign_<id>_at) and the bar value
// of a sign without a bar (hud_sign_<id>_bar).
constexpr const char* GODOT_HUD_SIGN_AT_CROWN = "crown";              // the mockup's crownPt(x, y) over the trunk
constexpr const char* GODOT_HUD_SIGN_AT_TRUNK = "trunk";              // an offset from the foot of the trunk
constexpr const char* GODOT_HUD_SIGN_AT_BROOK = "brook";              // the share x along the brook, y above it
constexpr const char* GODOT_HUD_SIGN_AT_SCREEN = "screen";            // a point of the mockup's screen
constexpr const char* GODOT_HUD_SIGN_BAR_NONE = "none";               // the sign has no bar

// The weather kinds sky.weather.v1 names, as the mockup's S.weather; the veil (WeatherVeil) draws none of the
// first three, the sky band (SkyBand) gives every kind its day and night glyph.
constexpr const char* GODOT_HUD_WEATHER_CLEAR = "clear";              // an open sky
constexpr const char* GODOT_HUD_WEATHER_PCLOUD = "pcloud";            // a sky partly clouded
constexpr const char* GODOT_HUD_WEATHER_CLOUDY = "cloudy";            // a sky clouded over
constexpr const char* GODOT_HUD_WEATHER_FOG = "fog";                  // bands of mist over the horizon
constexpr const char* GODOT_HUD_WEATHER_RAIN = "rain";                // streaks leaning with the wind
constexpr const char* GODOT_HUD_WEATHER_DRIZZLE = "drizzle";          // short fine dashes
constexpr const char* GODOT_HUD_WEATHER_SNOW = "snow";                // swaying flakes

// The parts of the care view a touch can land on, named as the mockup's data-act (usecase:101440, 06.7 BagWindow).
constexpr const char* GODOT_HUD_ACT_PACK_GRIP = "win:ranzen";         // the leather pack's flap: drags the window
constexpr const char* GODOT_HUD_ACT_POUCH_GRIP = "win:bolsa";         // the linen pouch's neck: drags the window
constexpr const char* GODOT_HUD_ACT_WINDOW_BODY = "winbody";          // a window's body: takes the touch, does nothing
constexpr const char* GODOT_HUD_ACT_PACK_CLOSE = "bag";               // the pack's cross (and the bag icon): opens or closes it
constexpr const char* GODOT_HUD_ACT_POUCH_CLOSE = "pclose";           // the pouch's cross: closes it
constexpr const char* GODOT_HUD_ACT_ITEM = "item";                    // a thing; its key goes with it
constexpr const char* GODOT_HUD_CONTAINER_PACK = "ranzen";            // the leather pack as a place things lie in
constexpr const char* GODOT_HUD_CONTAINER_POUCH = "bolsa";            // the linen pouch as a place things lie in
constexpr const char* GODOT_HUD_CONTAINER_BELT_FIRST = "belt0";       // the left holster on the belt
constexpr const char* GODOT_HUD_CONTAINER_BELT_SECOND = "belt1";      // the right holster on the belt
constexpr const char* GODOT_HUD_ACT_NOOP = "noop";                    // an empty holster: takes the touch, does nothing
constexpr const char* GODOT_HUD_ACT_REST = "rest";                    // the scrim of the tool rack's list: back to the mode rest
constexpr const char* GODOT_HUD_ACT_SKY = "sky";                      // the sky band: opens the calendar sheet
constexpr const char* GODOT_HUD_ACT_DEBUG = "dbg";                    // the sky band's debug switch: shows or hides the debug layer
constexpr const char* GODOT_HUD_ITEM_ART_PACK = "ranzen";             // item_art.json: the drawing of the leather pack (BagIcon)

// Drag and drop (usecase:101440, 06.7 DragLayer, the mockup's dropTarget and dropItem): the places a thing can go
// besides a container (tg.c), the place words of texts.hud (tg.lab), the words and colours of the toast after a
// release, and what a release did.
constexpr const char* GODOT_HUD_PLACE_HAND = "hand";                  // the hand slot: the thing is equipped
constexpr const char* GODOT_HUD_PLACE_GROUND = "ground";              // the ground of the world, the tool place included
constexpr const char* GODOT_HUD_WHERE_HAND = "where.hand";            // texts.hud: in the hand
constexpr const char* GODOT_HUD_WHERE_BELT = "where.belt";            // texts.hud: on the belt, either holster
constexpr const char* GODOT_HUD_WHERE_PACK = "where.ranzen";          // texts.hud: in the leather pack
constexpr const char* GODOT_HUD_WHERE_POUCH = "where.bolsa";          // texts.hud: in the linen pouch
constexpr const char* GODOT_HUD_WHERE_RACK = "where.rack";            // texts.hud: at the tool place
constexpr const char* GODOT_HUD_WHERE_GROUND = "where.ground";        // texts.hud: on the ground
constexpr const char* GODOT_HUD_WHERE_TRUNK = "where.trunk";          // texts.hud: on the trunk, the vessel under the tap
constexpr const char* GODOT_HUD_WORD_DROP_AT = "drop_at";             // texts.hud: {item} · {where} after an accepted drop
constexpr const char* GODOT_HUD_WORD_FIT = "fit_";                    // texts.hud: the head of a refusal word fit_<c>
constexpr const char* GODOT_HUD_WORD_ITEM_HEAD = "items.";            // texts.hud: the head of an item's word items.<id>.
constexpr const char* GODOT_HUD_WORD_ITEM_TAIL = ".name";             // texts.hud: the tail of an item's word .<field>
constexpr const char* GODOT_HUD_WORD_ITEM_CASE = ".case";             // texts.hud: the tail of an item's case items.<id>.case
constexpr const char* GODOT_HUD_COLOUR_DROP = "DOMAIN_GROUND";        // the toast of an accepted drop (toast(..., 'ground'))
constexpr const char* GODOT_HUD_COLOUR_REFUSED = "DOMAIN_ALERT";      // the toast of a refused drop (toast(tg.bad, 'alert'))
constexpr const char* GODOT_HUD_DRAG_TAP = "tap";                     // the finger never moved past the slop: a tap on the thing
constexpr const char* GODOT_HUD_DRAG_SEND = "send";                   // the module allowed the place: the move goes to flora.stow.v1
constexpr const char* GODOT_HUD_DRAG_REFUSED = "refused";             // the module refused the place: its word was toasted
constexpr const char* GODOT_HUD_DRAG_HELD = "held";                   // no place under the finger or no verdict: the thing stays

}  // namespace ase::adp::godot
