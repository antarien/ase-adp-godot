#pragma once

/**
 * ASE GODOT ADAPTER - THE BROOK OF THE CARE VIEW WITH THE SKY IT REFLECTS AND THE STROKES OF ITS CURRENT
 *
 * @file        godot_plant_brook_reflection.hpp
 * @brief       The sky the water of the care view's 3D scene reflects, and the light strokes running along its course
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.4, row N07. The mockup fills the brook with drawWater
 *              (m.html 429 to 433): a gradient from the horizon's colour, turned toward the water's own far tone, at
 *              its far reach to the colour of the sky above, turned toward the near tone, at its near reach; over it
 *              five light strokes run along the middle line of the brook. BrookReflection
 *              (godot_hud_brook_reflection.hpp) derives the colours and measures as the shade; this unit hands them
 *              to the water material of GODOT_WATER_SHADER_FILE as global shader parameters, with the course of the
 *              brook the strokes follow. Far and near are the depth z of the ground (row N06): the shader reads the
 *              GODOT_GROUND_ROWS global light_plant_ground_season sets, the screen rows of z 0 and z 1, so the water
 *              and the rows of the ground share one measure of depth. The mockup's fill is opaque; the operator's order keeps
 *              the brook water murky green, so the reflection is light added on top at the shade's reflect_share and
 *              never takes anything of the water's own colour. Nothing of the water is computed here.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    render/pipeline/stage
 * @usecase     usecase:101339 - show the ground around plant
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-12
 * @modified    2026-11-12
 * @version     00.00.01.00002
 */

#include <godot_cpp/variant/color.hpp>

namespace ase::adp::godot {

// The global shader parameters of the brook's reflection; project.godot declares each under [shader_globals], since a
// shader that reads a global the project does not declare does not compile. They stand here and not in types.hpp,
// which nearly every unit of the adapter includes.
constexpr const char* GODOT_BROOK_FAR_COLOR = "brook_far_color";        // global: the reflected horizon with the far tone, at depth z 0
constexpr const char* GODOT_BROOK_NEAR_COLOR = "brook_near_color";      // global: the reflected sky above with the near tone, at depth z 1
constexpr const char* GODOT_BROOK_REFLECT_MIX = "brook_reflect_mix";    // global: share the reflection takes of the water's own colour, 0 before the HUD stands
constexpr const char* GODOT_BROOK_STREAK_COLOR = "brook_streak_color";  // global: colour and opacity of the strokes of the current
constexpr const char* GODOT_BROOK_STREAK = "brook_streak";              // global: count, gap, length and speed of the strokes, shares of the course
constexpr const char* GODOT_BROOK_STREAK_PX = "brook_streak_px";        // global: width of a stroke on the screen, pixels; 0 draws none
constexpr const char* GODOT_BROOK_START = "brook_start";                // global: start of the course the strokes follow, world x and z
constexpr const char* GODOT_BROOK_END = "brook_end";                    // global: end of that course, world x and z
constexpr const char* GODOT_BROOK_HALF = "brook_half";                  // global: half width of the bed, metres; the shader finds the bed's axis as the middle of the wet strip within it
constexpr float GODOT_BROOK_NOTE_STEP = 0.01f;                          // a share, width or place that moves less than this since the frame before writes no new log line
constexpr const char* GODOT_BROOK_NOTE_META = "ase_brook_noted";        // meta of the RenderingServer: the values the last log line named; Godot answers global_shader_parameter_get only in the editor

/**
 * The brook's reflection as BrookReflection::shade derives it for this frame (m.html drawWater 429 to 433). The
 * defaults leave the water as its own colours paint it: no reflection, no strokes.
 */
struct BrookReflectionShade {
    bool shown = false;              // false before the HUD stands: the water keeps its own colour and draws no strokes
    float reflect_share = 0.0f;      // brook_reflect_share: share of the reflected sky added as light over the murky green
    ::godot::Color far;              // the horizon's colour toward BROOK_TONE_FAR by brook_tone_far_share
    ::godot::Color near;             // the colour of the sky above toward BROOK_TONE_NEAR by brook_tone_near_share
    ::godot::Color streak;           // BROOK_STREAK, its alpha the strokes' opacity
    float streak_count = 0.0f;       // brook_streak_count
    float streak_gap = 0.0f;         // brook_streak_gap: shares of the course between two strokes
    float streak_length = 0.0f;      // brook_streak_length: share of the course one stroke covers
    float streak_speed = 0.0f;       // brook_streak_speed: share of the course a stroke moves per second
    float streak_px = 0.0f;          // brook_streak_width in mockup units times the HUD frame's pixels per unit
    float start_x = 0.0f;            // start of the course, world metres east
    float start_z = 0.0f;            // start of the course, world metres north
    float end_x = 0.0f;              // end of the course, world metres east
    float end_z = 0.0f;              // end of the course, world metres north
    float half_m = 0.0f;             // half width of the bed, metres (the stream record's half_m); 0 draws no strokes
};

/**
 * Hands `shade` to the water material (the GODOT_BROOK_* globals above): the far and near colours of the reflection
 * and its share, the strokes and the course they run along. A shade not shown or a course without length hands the
 * defaults, so the water stays as its own colours paint it. The view calls it every frame after
 * light_plant_ground_season, whose depth the reflection reads.
 */
void light_plant_brook_reflection(const BrookReflectionShade& shade);

}  // namespace ase::adp::godot
