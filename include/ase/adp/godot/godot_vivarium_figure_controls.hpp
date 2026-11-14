#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM FIGURE CONTROLS
 *
 * @file        godot_vivarium_figure_controls.hpp
 * @brief       VivariumFigureControls - the phone's stick, orbit drag, turn arc, zoom rail and the figure's focus
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.4, 07.5, 07.9 and 07.11 (VIVARIUM_PHASE_07_FIGURE_V1): the phone
 *              is held upright. While the player camera stands, the left half under the world takes the stick where
 *              the finger comes down, the right half turns the player camera around the figure with a horizontal
 *              drag, and the rail cam.zoom at its right edge shows the camera's distance (FigureZoom); a finger on the
 *              rail or two fingers pinching on that half set it. The arc fig.turn
 *              on the ground before the feet turns the standing figure on the spot (FigureTurn). A tap on the figure
 *              or on the arc opens its focus (FigureFocus): the comb soul.mode over the head hands the figure to the
 *              AI (AI/PC) and takes it back (SS/PC). The small square cam.mode under the debug switch (FigureSwitch)
 *              switches the fixed view of Phase 06 and the player camera in both directions.
 *
 *              The record holds the hud_figure_* measures of SHA in the mockup's units, the colours and words the
 *              stick draws with, the units of the arc, the rail, the focus and the switch, the figure's place and
 *              height of the frame, the camera's distance the rail drew last, and the fingers that hold them;
 *              godot_vivarium_figure.cpp reads the stick, godot_vivarium_figure_controls.cpp binds and draws the
 *              record, godot_vivarium_figure_input.cpp writes the fingers.
 *
 *              It stands apart from godot_vivarium_node.hpp because only the figure's unit changes it: a new measure
 *              of the stick recompiles the view's class through this header, never the records of the configuration,
 *              the layout or the HUD.
 *
 *              NOTHING HERE MOVES THE FIGURE: the record turns a finger into the stick's deflection, a drag into a
 *              turn of the camera or of the figure; ase-player moves and turns the figure and ase-camera places its
 *              camera through the ports of ase-pl-avatar.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_021
 * @category    ecs/module
 * @usecase     usecase:101554 - receive player input
 * @actor       Player Input Warden (PLAYINPWAR)
 * @created     2026-11-13
 * @modified    2026-11-14
 * @version     00.00.03.00001
 */

#include <ase/adp/godot/godot_hud_figure_focus.hpp>
#include <ase/adp/godot/godot_hud_figure_switch.hpp>
#include <ase/adp/godot/godot_hud_figure_turn.hpp>
#include <ase/adp/godot/godot_hud_figure_zoom.hpp>
#include <ase/adp/godot/godot_hud_ui_text_block.hpp>

#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <cstdint>

namespace ase::adp::godot {

/**
 * The phone's controls of the figure: the hud_figure_* measures of SHA in the mockup's units, the word of the resting
 * stick, the arc fig.turn, the rail cam.zoom, the figure's focus with soul.mode, the switch cam.mode, and the fingers
 * that hold them. A finger is its touch index; the desktop's left mouse button is one finger of its own.
 */
struct VivariumFigureControls {
    static constexpr int64_t NO_FINGER = -1;     // no finger holds it
    static constexpr int64_t MOUSE_FINGER = -2;  // the left mouse button of the desktop holds it

    ::godot::Rect2   stick_zone;                 // hud_figure_stick_zone_*: the left half, a finger there takes the stick
    ::godot::Rect2   orbit_zone;                 // hud_figure_orbit_zone_*: the right half, a drag there turns the camera
    ::godot::Vector2 stick_rest;                 // hud_figure_stick_rest_x/_y: the centre of the resting ring
    double stick_radius = 0.0;                   // hud_figure_stick_radius: the full deflection of the stick
    double knob_radius = 0.0;                    // hud_figure_stick_knob_radius
    double stick_line = 0.0;                     // hud_figure_stick_line: the stroke of the ring
    double rest_alpha = 0.0;                     // hud_figure_stick_rest_alpha: the resting ring's opacity
    double rest_dash = 0.0;                      // hud_figure_stick_rest_dash: the dash of the resting ring
    double rest_gap = 0.0;                       // hud_figure_stick_rest_gap: the gap between its dashes
    double rest_label_dy = 0.0;                  // hud_figure_stick_rest_label_dy: the word's baseline below the centre
    double rest_label_size = 0.0;                // hud_figure_stick_rest_label_size
    double orbit_yaw_per_unit = 0.0;             // hud_figure_orbit_yaw_per_unit: radians per mockup unit dragged
    double walk_share = 0.0;                     // hud_figure_keys_walk_share: the deflection of full walking pace
    double run_share = 0.0;                      // hud_figure_keys_run_share: the deflection from which the figure runs
    double key_turn_rate = 0.0;                  // hud_figure_keys_turn_rate: arrow keys, radians of camera yaw per second
    double key_pitch_rate = 0.0;                 // hud_figure_keys_pitch_rate: arrow keys, radians of camera pitch per second
    double key_spin_rate = 0.0;                  // hud_figure_keys_spin_rate: Q and E, radians the figure turns per second
    double key_zoom_rate = 0.0;                  // hud_figure_keys_zoom_rate: Z and X, metres per second along the rail
    ::godot::Color   stick_ground;               // FIGURE_STICK_GROUND: the ring's fill while a thumb holds the stick
    ::godot::Color   stick_ring;                 // FIGURE_STICK_RING: the outline of that ring
    ::godot::Color   stick_knob;                 // FIGURE_STICK_KNOB: the knob under the thumb
    ::godot::Color   stick_rest_ring;            // FIGURE_STICK_REST: the dashed ring where the stick rests
    ::godot::Color   stick_rest_word;            // FIGURE_STICK_REST_LABEL: the word inside the resting ring
    UiTextBlock      stick_word;                 // figure.stick_rest: under the resting ring
    int64_t          stick_finger = NO_FINGER;   // the finger on the stick
    ::godot::Vector2 stick_origin;               // where it came down, the stick's centre, mockup units
    ::godot::Vector2 stick_at;                   // where it is, at most stick_radius off the origin
    int64_t          orbit_finger = NO_FINGER;   // the finger dragging the orbit
    float            orbit_last_x = 0.0f;        // its x at the last drag, mockup units
    bool             orbit_still = false;        // it is the finger a pinch left behind: it turns nothing, it may pinch again
    ::godot::Vector2 orbit_at;                   // where it is, mockup units: one end of a pinch
    int64_t          pinch_finger = NO_FINGER;   // a second finger on the orbit half: the two pinch cam.zoom
    ::godot::Vector2 pinch_at;                   // where it is, mockup units: the other end of the pinch
    double           pinch_spread = 0.0;         // the two fingers' spread when the second came down, mockup units
    double           pinch_from = 0.0;           // the camera's distance then, metres
    double           pinch_asked = 0.0;          // the distance the pinch asked for last, metres
    bool             camera_drawn = false;       // the rail stood in the last frame at the camera record's distance
    double           camera_metres = 0.0;        // that distance from the eye to the aim, metres, valid while camera_drawn
    uint32_t         led = 0;                    // the object of the figure this seat leads, valid while led_known
    bool             led_known = false;          // the figure of this seat stood in the last frame
    bool             led_self = false;           // its soul is led by its player (SS/PC); false: the AI leads it (AI/PC)
    ::godot::Vector3 led_at;                     // the figure's place in world metres, as the body port reported it
    double           led_height = 0.0;           // its height in metres (figure_height of the scene); 0 while unbuilt
    FigureSwitch     cam_switch;                 // cam.mode: the switch under the debug switch, in every camera
    FigureFocus      focus;                      // fig.focus: the comb soul.mode over the head, the spine right of it
    FigureTurn       turn;                       // fig.turn: the arc on the ground before the feet
    FigureZoom       zoom;                       // cam.zoom: the rail of the camera's distance, while the player camera stands
    bool             focus_open = false;         // a tap on the figure or the arc opened its focus; the next tap closes it
    double           focus_since = 0.0;          // the HUD's clock when it opened
    int64_t          turn_finger = NO_FINGER;    // the finger drawing on the arc
    float            turn_last_x = 0.0f;         // its x at the last move, mockup units
    int64_t          zoom_finger = NO_FINGER;    // the finger on the rail
    float            zoom_last_y = 0.0f;         // its y at the last move, mockup units
    double           tap_slop = 0.0;             // hud_gesture_press_slop: a finger wandering further makes no tap
    double           tap_s = 0.0;                // hud_gesture_tap_ms in seconds: a longer touch is no tap
    int64_t          tap_finger = NO_FINGER;     // the finger whose touch may still be a tap
    ::godot::Vector2 tap_at;                     // where it came down, mockup units
    double           tap_since = 0.0;            // the HUD's clock when it came down
};

}  // namespace ase::adp::godot
