/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW FIGURE CONTROLS
 *
 * @file        godot_vivarium_figure_controls.cpp
 * @brief       VivariumView - the measures and the drawing of the phone's stick, turn arc, zoom rail, focus and cam.mode
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.6, 07.7, 07.9 and 07.11 (VIVARIUM_PHASE_07_FIGURE_V1, drawHud): the
 *              phone is held upright. While the player camera stands, the stick rests as a dashed ring on the left
 *              half under the world and stands under the thumb that holds it, the arc fig.turn lies on the ground
 *              before the feet, and the rail cam.zoom at the right edge carries its knob at the camera's distance. The
 *              focus fig.focus with the comb soul.mode stands about the figure in every camera while a tap opened it,
 *              and the square cam.mode under the debug switch is drawn in both.
 *
 *              Every measure, colour and word is SHA's (hud_figure_*, hud_gesture_*, FIGURE_*, figure.*), read once
 *              with the HUD (load_figure_hud) in the mockup's units of a 360 wide frame; hud_frame_ carries them onto
 *              the canvas. draw_figure_hud lays the frame's figure from the body port and the scene's height of its
 *              node, and keeps the camera's distance the rail drew in the record, where the touch reads it.
 *
 *              It stands apart from godot_vivarium_figure.cpp because its reason to change is the phone's surface:
 *              a new zone or a new unit of the controls, never the figure's configuration or its body; and apart from
 *              godot_vivarium_figure_input.cpp, which turns the fingers into the ports of ase-pl-avatar. Nothing
 *              about the figure is computed here; ase-player moves and turns it, ase-camera places its camera.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_021
 * @category    ecs/module
 * @usecase     usecase:101554 - receive player input
 * @actor       Player Input Warden (PLAYINPWAR)
 * @created     2026-11-13
 * @modified    2026-11-14
 * @version     00.00.03.00003
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_figure_scene.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_paint.hpp>
#include <ase/adp/godot/godot_hud_ui_svg_path.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/math/math.hpp>

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace ase::adp::godot {

namespace {

/** The circle of `radius` about `centre`, closed into `outline` at the ring's point count of a full turn. */
void append_circle(UiSvgOutline& outline, const ::godot::Vector2& centre, double radius) {
    const int32_t count = static_cast<int32_t>(GODOT_RING_POINTS_PER_TURN);
    for (int32_t index = 0; index < count; ++index) {
        const float angle = 2.0f * ase::math::PI * static_cast<float>(index) / static_cast<float>(count);
        svg_append(outline, centre + ::godot::Vector2(ase::math::cos(angle), ase::math::sin(angle)) *
                                         static_cast<float>(radius));
    }
    outline.closed = true;
}

/** The distance in metres from the eye of the player camera to its aim, as its record carries them. */
double camera_distance(const VivariumFigureCameraView& camera) {
    const ::godot::Vector3 eye(static_cast<float>(camera.eye_x), static_cast<float>(camera.eye_y),
                               static_cast<float>(camera.eye_z));
    const ::godot::Vector3 aim(static_cast<float>(camera.aim_x), static_cast<float>(camera.aim_y),
                               static_cast<float>(camera.aim_z));
    return static_cast<double>(eye.distance_to(aim));
}

}  // namespace

bool VivariumView::load_figure_hud(const UiTheme& theme) {
    // THE PHONE'S CONTROLS (07.9): every measure of the stick, the orbit and the tap is a hud_* token of SHA in the
    // mockup's units; a missing one stands on the error panel like every other HUD token.
    VivariumFigureControls& controls = figure_controls_;
    double zone[8] = {};
    double rest[2] = {};
    double tap_ms = 0.0;
    for (const std::pair<const char*, double*>& slot : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_figure_stick_zone_x", &zone[0]},          {"hud_figure_stick_zone_y", &zone[1]},
             {"hud_figure_stick_zone_w", &zone[2]},          {"hud_figure_stick_zone_h", &zone[3]},
             {"hud_figure_orbit_zone_x", &zone[4]},          {"hud_figure_orbit_zone_y", &zone[5]},
             {"hud_figure_orbit_zone_w", &zone[6]},          {"hud_figure_orbit_zone_h", &zone[7]},
             {"hud_figure_stick_rest_x", &rest[0]},          {"hud_figure_stick_rest_y", &rest[1]},
             {"hud_figure_stick_radius", &controls.stick_radius},
             {"hud_figure_stick_knob_radius", &controls.knob_radius},
             {"hud_figure_stick_line", &controls.stick_line},
             {"hud_figure_stick_rest_alpha", &controls.rest_alpha},
             {"hud_figure_stick_rest_dash", &controls.rest_dash},
             {"hud_figure_stick_rest_gap", &controls.rest_gap},
             {"hud_figure_stick_rest_label_dy", &controls.rest_label_dy},
             {"hud_figure_stick_rest_label_size", &controls.rest_label_size},
             {"hud_figure_orbit_yaw_per_unit", &controls.orbit_yaw_per_unit},
             {"hud_figure_keys_walk_share", &controls.walk_share},
             {"hud_figure_keys_run_share", &controls.run_share},
             {"hud_figure_keys_turn_rate", &controls.key_turn_rate},
             {"hud_figure_keys_pitch_rate", &controls.key_pitch_rate},
             {"hud_figure_keys_spin_rate", &controls.key_spin_rate},
             {"hud_figure_keys_zoom_rate", &controls.key_zoom_rate},
             {"hud_gesture_press_slop", &controls.tap_slop},
             {"hud_gesture_tap_ms", &tap_ms},
             {"hud_figure_body_turn_rate", &figure_look_.motion.turn_rate},
             {"hud_figure_body_turn_full", &figure_look_.motion.turn_full},
             {"hud_figure_body_look_lift", &figure_look_.motion.look_lift}}) {
        if (!theme.measure(slot.first, *slot.second) || !std::isfinite(*slot.second)) {
            return fail_config(slot.first);
        }
    }
    if (!(controls.stick_radius > 0.0) || !(controls.walk_share > 0.0)) {
        return fail_config("hud_figure_stick_radius");  // a stick without reach or pace leads no figure
    }
    if (!(figure_look_.motion.turn_rate > 0.0) || !(figure_look_.motion.turn_full > 0.0)) {
        return fail_config("hud_figure_body_turn_rate");  // a body without a turn never follows the course of its port
    }
    if (!(figure_look_.motion.look_lift > 0.0)) {
        return fail_config("hud_figure_body_look_lift");  // the distance to the feet would pick the detail level too fine
    }
    if (!(controls.key_turn_rate > 0.0) || !(controls.key_pitch_rate > 0.0) || !(controls.key_spin_rate > 0.0) ||
        !(controls.key_zoom_rate > 0.0)) {
        return fail_config("hud_figure_keys_turn_rate");  // a key without a rate moves nothing on the desktop
    }
    if (!(tap_ms > 0.0)) {
        return fail_config("hud_gesture_tap_ms");  // a tap without time is never a tap
    }
    controls.tap_s = tap_ms / GODOT_MS_PER_SECOND;
    controls.stick_zone = ::godot::Rect2(static_cast<float>(zone[0]), static_cast<float>(zone[1]),
                                         static_cast<float>(zone[2]), static_cast<float>(zone[3]));
    controls.orbit_zone = ::godot::Rect2(static_cast<float>(zone[4]), static_cast<float>(zone[5]),
                                         static_cast<float>(zone[6]), static_cast<float>(zone[7]));
    controls.stick_rest = ::godot::Vector2(static_cast<float>(rest[0]), static_cast<float>(rest[1]));

    for (const std::pair<const char*, ::godot::Color*>& slot :
         std::initializer_list<std::pair<const char*, ::godot::Color*>>{
             {"FIGURE_STICK_GROUND", &controls.stick_ground},
             {"FIGURE_STICK_RING", &controls.stick_ring},
             {"FIGURE_STICK_KNOB", &controls.stick_knob},
             {"FIGURE_STICK_REST", &controls.stick_rest_ring},
             {"FIGURE_STICK_REST_LABEL", &controls.stick_rest_word}}) {
        if (!theme.color(slot.first, *slot.second)) {
            return fail_config(slot.first);
        }
    }

    // the resting stick's word in the label face; the arc, the rail, the focus and the switch bind their own
    std::string failed;
    std::vector<HudRun> runs;
    if (!controls.stick_word.bind_label(theme, failed)) {
        return fail_config(("hud " + failed).c_str());
    }
    if (!theme.words("figure.stick_rest", runs) || !controls.stick_word.set(runs, failed)) {
        return fail_config(failed.empty() ? "figure.stick_rest" : failed.c_str());
    }
    if (!controls.cam_switch.bind(theme, failed) || !controls.focus.bind(theme, failed) ||
        !controls.turn.bind(theme, failed) || !controls.zoom.bind(theme, failed)) {
        return fail_config(("hud " + failed).c_str());
    }
    return true;
}

void VivariumView::draw_figure_hud(UiPen& pen) {
    VivariumFigureControls& controls = figure_controls_;
    controls.camera_drawn = false;  // until the rail stands again in this frame
    if (!controls.led_known) {
        // no figure of this seat: nothing to press, nothing open, no finger holds anything
        controls.focus_open = false;
        controls.focus.close();
        controls.turn.clear();
        for (int64_t* held : {&controls.stick_finger, &controls.orbit_finger, &controls.turn_finger,
                              &controls.zoom_finger, &controls.pinch_finger, &controls.tap_finger}) {
            *held = VivariumFigureControls::NO_FINGER;
        }
        return;
    }
    if (!figure_view_shown_) {
        // the fixed view stands: the arc, the rail and the orbit half of the player camera hold no finger
        for (int64_t* held : {&controls.orbit_finger, &controls.turn_finger, &controls.zoom_finger,
                              &controls.pinch_finger}) {
            *held = VivariumFigureControls::NO_FINGER;
        }
    }
    UiTextPlace text;
    text.anchor = GODOT_TEXT_ANCHOR_MIDDLE;
    text.still = hud_still_;
    if (figure_view_shown_) {
        pen.draw_set_transform_matrix(hud_frame_);
        if (controls.stick_finger != VivariumFigureControls::NO_FINGER) {
            // the stick under the thumb: its ring where the thumb came down, the knob where it is
            UiSvgOutline ring;
            append_circle(ring, controls.stick_origin, controls.stick_radius);
            svg_fill(pen, ring, controls.stick_ground);
            svg_stroke(pen, ring, controls.stick_ring, controls.stick_line);
            pen.draw_circle(controls.stick_at, static_cast<float>(controls.knob_radius), controls.stick_knob, true);
            pen.draw_set_transform_matrix(::godot::Transform2D());
        } else {
            // the resting stick: a dashed ring with its word, the group at rest_alpha
            UiSvgOutline ring;
            append_circle(ring, controls.stick_rest, controls.stick_radius);
            ::godot::Color dashed = controls.stick_rest_ring;
            dashed.a *= static_cast<float>(controls.rest_alpha);
            svg_stroke_dashed(pen, ring, dashed, controls.stick_line, controls.rest_dash, controls.rest_gap);
            pen.draw_set_transform_matrix(::godot::Transform2D());
            text.at = controls.stick_rest + ::godot::Vector2(0.0f, static_cast<float>(controls.rest_label_dy));
            text.size = controls.rest_label_size;
            text.color = controls.stick_rest_word;
            text.opacity = controls.rest_alpha;
            controls.stick_word.draw(pen, hud_frame_, text);
        }
    }

    // THE FIGURE OF THIS FRAME: its place as the body port reported it, its height as the scene measured its node
    bool placed = false;
    for (const VivariumFigureView& seen : resources_.get_figures()) {
        if (seen.object_id == controls.led) {
            controls.led_at = ::godot::Vector3(static_cast<float>(seen.x), static_cast<float>(seen.y),
                                               static_cast<float>(seen.z));
            placed = true;
            break;
        }
    }
    double height = 0.0;
    controls.led_height = placed && figure_height(scene_, controls.led, height) ? height : 0.0;
    ::godot::Vector3 eye;
    ::godot::Vector2 foot;
    ::godot::Vector2 head;
    const bool shown = placed && hud_anchor_.eye(eye) && hud_anchor_.place(controls.led_at, foot) &&
                       hud_anchor_.place(controls.led_at + ::godot::Vector3(0.0f, static_cast<float>(controls.led_height),
                                                                            0.0f),
                                         head);
    if (shown) {
        // fig.turn under the player camera, laid toward the eye; fig.focus about the figure in every camera
        if (figure_view_shown_ && controls.turn.lay(hud_anchor_, controls.led_at, eye)) {
            controls.turn.draw(pen, hud_frame_, controls.focus_open,
                               controls.turn_finger != VivariumFigureControls::NO_FINGER);
        } else {
            controls.turn.clear();
        }
        FigureFocusFrame now;
        now.foot = foot;
        now.head = head;
        now.led_self = controls.led_self;
        now.elapsed = hud_clock_ - controls.focus_since;
        now.still = hud_still_;
        controls.focus.lay(now);
        if (controls.focus_open) {
            std::string failed;
            if (!controls.focus.draw(pen, hud_frame_, now, failed)) {
                fail_hud(failed);
                return;
            }
        }
    } else {
        // the figure stands behind the camera or not yet in the scene: nothing about it to draw or to hit
        controls.turn.clear();
        if (controls.focus_open) {
            controls.focus_open = false;
            controls.focus.close();
        }
    }

    // cam.zoom while the player camera stands and its record carries the eye and the aim; the touch reads the
    // distance it drew, so the rail's jump and the pinch start where the knob stands
    const VivariumFigureCameraView& camera = resources_.get_figure_camera();
    if (figure_view_shown_ && camera.known) {
        controls.camera_metres = camera_distance(camera);
        controls.camera_drawn = true;
        std::string failed;
        const bool held = controls.zoom_finger != VivariumFigureControls::NO_FINGER ||
                          controls.pinch_finger != VivariumFigureControls::NO_FINGER;
        if (!controls.zoom.draw(pen, hud_frame_, controls.camera_metres, held, failed)) {
            fail_hud(failed);
            return;
        }
    }

    // cam.mode under the debug switch, in every camera
    controls.cam_switch.draw(pen, hud_frame_, figure_view_shown_);
}

}  // namespace ase::adp::godot
