#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW CONFIGURATION
 *
 * @file        godot_vivarium_view_config.hpp
 * @brief       The records VivariumView reads from res://config/vivarium_start.json: layout, run modes, texts, verbs
 * @description What the start configuration says, as the view holds it after load: where every rectangle and text
 *              line stands on the logical surface (VivariumLayout), the run modes a launch argument switches on
 *              (VivariumModes), every text the view draws (VivariumTexts) and what each ground tool of hud.verbs
 *              does (VivariumHudVerb). The view reads them once at configure time and never writes them back; no
 *              number of them stands in the code, every one comes from the configuration (godot_vivarium_view.cpp,
 *              godot_vivarium_hud.cpp).
 *
 *              ONE RESPONSIBILITY, ITS OWN HEADER. These are the configuration's records; the view's runtime state
 *              and its entry points stay in godot_vivarium_node.hpp, which includes this header. A unit that needs
 *              a record and not the view reads it from here.
 *
 *              THE VERB'S TOAST IS DATA. applyTerr of the HUD mockup (m.html 979 to 991) shows one word per verb
 *              once it acted, in the domain colour of the verb's tone; furar and soltar share the op loosen and
 *              differ only by that word. So the word and the tone stand in hud.verbs.<name>.toast and .tone, and
 *              send_hud_verb (godot_vivarium_hud_input.cpp) reads them instead of telling the verbs apart by a
 *              literal.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066, DSGN_067
 * @category    ecs/module
 * @usecase     usecase:100387 - view the plant
 * @actor       Plant Keeper (PLNTKEEPER)
 * @register    carrier N0448
 * @created     2026-11-12
 * @modified    2026-11-21
 * @version     00.00.22.00022 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_view_layout.hpp>

#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** What a ground tool of hud.verbs does: its terrain operation, the radius of its disc and the toast it acted with. */
struct VivariumHudVerb {
    std::string op;       // VivariumHud::OP_*: the operation the verb carries out
    double brush_m = 0.0; // its brush_m, or view.flatten_m where the verb names none
    double depth_m = 0.0; // its depth_m: what one spade stroke digs down or heaps up, metres; required for lower and raise
    std::string tone;     // its tone: the toast's colour is DOMAIN_ plus it (applyTerr's 'ground' or 'water')
    std::string toast;    // the SHA word of the toast once it acted (applyTerr: toast.lowered, toast.holed, ...)
    std::string roots;    // the SHA word of the warning when the brush overlaps the root zone; empty: the verb warns of nothing
};

/**
 * Where things stand on the logical 720 x 1280 surface - read from
 * res://config/vivarium_start.json, never written into the code. Every rectangle and every text
 * line lies inside ONE of the three bands; load_layout refuses a configuration where one crosses a
 * band edge, because the bands move apart on a taller surface.
 */
struct VivariumLayout {
    ::godot::Vector2 size;
    ViewBands        bands;                // logical size and the two band edges (layout.bands)
    float            header_height = 0.0f;
    ::godot::Vector2 title_at;
    ::godot::Vector2 build_at;
    ::godot::Vector2 info_at;
    float            info_line = 0.0f;
    ::godot::Vector2 note_at;
    std::vector<::godot::Rect2> place_rects;  // the rectangle of each place, index = place
    ::godot::Rect2   water_rect;
    ::godot::Rect2   level_rect;               // the field that levels the selected patch (03.8 point 2)
    ::godot::Rect2   prune_rect;               // the button of the cutting mode (03.10 step 9)
    ::godot::Rect2   bed_rect;                 // the button of the bed mode: two taps on the ground lay a brook's bed (03.8)
    ::godot::Rect2   flatten_rect;             // the button of the flatten mode: a tap on the ground levels a disc there (03.8)
    ::godot::Rect2   reset_rect;
};

/**
 * The explicit run modes of the start configuration (PLAN_ASE_VIVARIUM_PHASE_02_ANDROID 02.3).
 * A mode runs only when the launch names its argument after `--` (Godot's user arguments; on
 * Android the intent extra command_line_params) - never on a normal start.
 */
struct VivariumModes {
    ::godot::String selftest_argument;       // runs the self-test table before the game host boots
    ::godot::String selftest_table;          // res:// path of the self-test table
    ::godot::String measure_argument;        // measures host and frame times after a warm-up
    double          measure_warmup_seconds = 0.0;  // foreground seconds before the recording starts
    double          measure_record_seconds = 0.0;  // foreground seconds recorded
    ::godot::String assets_argument;         // shows every asset of the list beside the game host (PLAN_ASE_VIVARIUM_PHASE_04_PIPELINE 04.7)
    ::godot::String debug_argument;          // shows the debug buttons: the time lapse and the jump of this host's clock (godot_vivarium_debug.hpp)
    ::godot::String weather_argument;        // the word after it is the stage's weather, a GODOT_HUD_WEATHER_* key the bridge writes for the artefact's (usecase:5606, load_hud_bridge)
    ::godot::String still_argument;          // holds every HUD motion still, as the platform's reduced-motion setting does (usecase:101440, read_hud_still)
    ::godot::String perception_argument;     // the number after it is the stage's perception level, one of hud.perception.levels, in place of hud.perception.start (usecase:101440, load_hud_bridge)
};

/** Every text the view draws, Brazilian Portuguese, from the same configuration. */
struct VivariumTexts {
    ::godot::String title;
    ::godot::String selection;
    ::godot::String selection_none;
    ::godot::String area;
    ::godot::String simulation;
    ::godot::String ticks;
    ::godot::String day;
    ::godot::String hour;
    ::godot::String coverage;
    ::godot::String soil;
    ::godot::String stage_seed;
    ::godot::String stage_sprout;
    ::godot::String stage_grow;
    ::godot::String stage_mature;
    ::godot::String condition_wilted;
    ::godot::String condition_dead;
    ::godot::String unknown;
    ::godot::String restart_note;
    ::godot::String error;
    ::godot::String water;
    ::godot::String level;
    ::godot::String prune;
    ::godot::String bed;
    ::godot::String flatten;
    ::godot::String reset;
    ::godot::String separator;   // between the parts of one line, e.g. "área 2 · Broto"
    ::godot::String loading;     // the line of the start screen while the world is initialised
    ::godot::String hold_patches;    // the step that holds the start screen, after its percent line (B0357): patches not announced
    ::godot::String hold_brooks;     // brooks not all carried
    ::godot::String hold_scene;      // scene parts not all built
    ::godot::String hold_sky_clock;  // sky and game clock not known
    ::godot::String hold_figure;     // the figure file still loading
    ::godot::String hold_jump;       // the frame jumped
    ::godot::String hold_render;     // pipelines compiling or the quiet frames not yet counted
};

}  // namespace ase::adp::godot
