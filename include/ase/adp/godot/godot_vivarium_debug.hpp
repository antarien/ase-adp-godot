#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM DEBUG RECORDS
 *
 * @file        godot_vivarium_debug.hpp
 * @brief       The records of the Vivarium's debug mode: the time lapse, the jump, the relief, the hour length
 * @description The debug mode of the Vivarium view (modes.debug of the start configuration) is drawn and
 *              answered by godot_vivarium_debug.cpp and godot_vivarium_clock.cpp; this header holds what
 *              those units read from the section "debug" and what they keep while a request runs. It
 *              stands apart from godot_vivarium_node.hpp because only the debug units change it: a new
 *              button kind or a field of the running jump recompiles the view's class, never the
 *              records of the configuration, the layout or the measurement.
 *
 *              TWO CASES OF TIME, NOTHING ELSE (operator order of 2026-11-06): the time lapse runs the
 *              host's clock at a multiple of ase-time's base pace, the factor travels through the pace
 *              port and ase-pl-flora multiplies it with the base pace read from the star
 *              (usecase:101120 clock the growth cycle). The jump moves the clock to a target - a stage
 *              of the selected patch or an hour of the game day - and every tick up to it is computed
 *              by every system of the host, across the day change too, while the pace stays as it was.
 *              Both run on the client's own clock; no server clock moves and nothing is loaded.
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
 * @modified    2026-11-06
 * @version     00.00.01.00001
 */

#include <ase/adp/godot/types.hpp>

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
 * the clock jumps to a target, and every tick up to it is computed by every system of the host, across the
 * day change too; the pace stays as it was before. The target is a stage of the selected patch or an hour of
 * the game day - the same case, only the recognition of the target differs.
 */
struct VivariumDebugJump {
    ::godot::Rect2   rect;
    ::godot::String  label;
    ::godot::String  icon;                     // the glyph of its ICON_* in SHA's icon table, empty: the label alone
    ::godot::Vector2 icon_offset;              // the offset Icon.tsx gives that glyph, CSS pixels at debug.icon_web_px
    uint8_t         stage = GODOT_STAGE_NONE;   // the stage the selected patch jumps to; NONE: the jump names an hour
    double          hour = GODOT_HOUR_NONE;     // the hour of the game day the clock jumps to; NONE: the jump names a stage
};

/**
 * The jump that runs (a VivariumDebugJump asked and granted). A stage jump ends once the watched patch
 * reached its stage or its stand died; an hour jump ends once the clock's game seconds reached target_s. A
 * jump to an hour behind the clock on day 0 cannot compute its target before the day turned: it runs on to
 * the day change with one game hour per feed at most, then computes target_s from the new day's start.
 */
struct VivariumJump {
    bool     running = false;                  // a jump runs
    uint32_t button = UINT32_MAX;              // index of its button in debug.jumps, drawn with the wider outline
    uint8_t  stage = GODOT_STAGE_NONE;         // the stage it ends at, NONE: an hour jump
    uint32_t place = GODOT_NO_PLACE;           // the place whose patch a stage jump watches
    double   hour = GODOT_HOUR_NONE;           // the hour of the game day an hour jump ends at
    double   target_s = GODOT_HOUR_NONE;       // game seconds of the clock an hour jump ends at, below 0: not computed yet
    double   asked_day = 0.0;                  // the game day the jump was asked on
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
 * A JUMP RUNS FASTER THAN THE FRAMES: on top of the frame's real time the view hands the host up to lapse_s
 * and lets it run lapse_steps more ticks in that frame (GodotHostResourceManager::advance). Every tick stays
 * an ordinary tick of the client's own clock - the client computes the years itself.
 */
struct VivariumDebug {
    bool            on = false;                 // launched with modes.debug.argument
    ::godot::Color  fill;                       // face of a debug button (debug.fill), unlike every game button
    ::godot::Color  text;                       // label and outline of a debug button (debug.text)
    ::godot::Ref<::godot::FontFile> icon_font;  // the Nerd Font of the button icons (debug.icon_font), loaded at run time
    double          icon_web_px = 0.0;          // the font size the offsets of the icon table are meant for (its web_size_px)
    double          keep_soil_rel = 0.0;        // during a stage jump the patch is watered below this share of its capacity
    double          lapse_s = 0.0;              // real seconds a frame of a jump hands the host beyond its own at most (debug.lapse_s)
    uint32_t        lapse_steps = 0u;           // ticks a frame of a jump may run beyond catch_up_steps (debug.lapse_steps)
    std::vector<VivariumDebugButton> buttons;   // the time lapse buttons (debug.buttons)
    std::vector<VivariumDebugJump> jumps;       // the jump buttons (debug.jumps)
    std::vector<VivariumDebugRelief> reliefs;   // the relief buttons (debug.reliefs)
    double          height_scale_m = 0.0;       // the height amplitude last granted by the elevation port, 0 = none asked yet
    double          factor = GODOT_PACE_NONE;   // the factor asked of the running host, GODOT_PACE_NONE = its default pace
    VivariumJump    jump;                       // the jump that runs
};

/**
 * THE LENGTH OF A GAME HOUR AND THE RATE OF THE CLOCK, measured in every frame from the clock port and the
 * host's simulated real time, two readings within one game day; the view carries no constant of ase-time.
 * The hour jump computes its target from the hour length and its feed from the rate.
 */
struct VivariumHourLength {
    double last_elapsed_s = -1.0;    // game seconds of the clock port in the previous frame, below 0: none yet
    double last_day = -1.0;          // game day of the clock port in the previous frame, below 0: none yet
    double last_hour = -1.0;         // hour of the clock port in the previous frame, below 0: none yet
    double seconds_per_hour = 0.0;   // game seconds of one game hour as measured, 0: not measured yet
    double last_simulated_s = -1.0;  // real seconds the host had simulated in the previous frame, below 0: none yet
    double game_per_real = 0.0;      // game seconds the clock runs per simulated real second, 0: not measured yet
};

}  // namespace ase::adp::godot
