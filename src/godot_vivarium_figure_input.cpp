/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW FIGURE INPUT
 *
 * @file        godot_vivarium_figure_input.cpp
 * @brief       VivariumView::figure_input - the phone's fingers on the stick, the orbit half, the arc, the rail and the taps
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.6, 07.7, 07.9 and 07.11 (VIVARIUM_PHASE_07_FIGURE_V1, pointerdown,
 *              pointermove, pointerup, pinch): the phone is held upright. While the player camera stands, a finger on
 *              the left half under the world takes the stick where it comes down, a horizontal drag on the right half
 *              turns the player camera around the figure, a finger on the rail cam.zoom at its right edge sets the
 *              camera's distance, and a finger drawing sideways on the arc fig.turn before the feet turns the
 *              standing figure on the spot.
 *
 *              THE PINCH. A second finger on the right half while the first one holds it starts a pinch from the
 *              distance the rail drew last; while both stand the distance follows their spread at the press over their
 *              spread now (FigureZoom::pinch), and the orbit turns nothing. When one of them lifts the pinch ends; the
 *              finger left behind turns nothing and starts the next pinch with the next finger, as the mockup keeps it
 *              among its fingers with the orbit off.
 *
 *              A TAP is a touch that wanders no further than hud_gesture_press_slop and lifts within
 *              hud_gesture_tap_ms. It meets, in this order: the square cam.mode under the debug switch, which switches
 *              the fixed view and the player camera; the comb soul.mode over the head while the focus stands open,
 *              which hands the figure to the AI or takes it back; the figure or its arc, which opens or closes the
 *              focus. A tap anywhere else closes the focus and is left to the HUD.
 *
 *              It stands apart from godot_vivarium_figure_controls.cpp because its reason to change is the touch: a new
 *              gesture, a new order of the taps or a new port a finger goes through, never a measure or a drawing of
 *              the controls. The drags, the rail, the pinch, the arc, the comb and the switch go through the ports of
 *              ase-pl-avatar here, at the touch; the stick's deflection is read by follow_figure
 *              (godot_vivarium_figure.cpp). Nothing about the figure is computed here: ase-player moves and turns it,
 *              ase-camera places its camera within its own limits.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_021
 * @category    ecs/module
 * @usecase     usecase:101554 - receive player input
 * @actor       Player Input Warden (PLAYINPWAR)
 * @created     2026-11-14
 * @modified    2026-11-14
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/hud/types.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>

#include <cstdint>
#include <initializer_list>

namespace ase::adp::godot {

namespace {

/**
 * The metres the pinch of this move adds to the distance it asked for last, which it keeps; 0 while the two fingers
 * stand too close to ask for anything.
 */
double pinch_delta(VivariumFigureControls& controls) {
    double asked = 0.0;
    if (!controls.zoom.pinch(controls.pinch_from, controls.pinch_spread,
                             static_cast<double>(controls.orbit_at.distance_to(controls.pinch_at)), asked)) {
        return 0.0;
    }
    const double delta = asked - controls.pinch_asked;
    controls.pinch_asked = asked;
    return delta;
}

}  // namespace

bool VivariumView::figure_input(const ::godot::Ref<::godot::InputEvent>& local) {
    VivariumFigureControls& controls = figure_controls_;
    if (!controls.led_known) {
        return false;
    }
    // One finger per touch index; the left mouse button is a finger of its own unless Godot already hands it on as
    // touch index 0, which would answer the same press twice.
    const ::godot::Input* input = ::godot::Input::get_singleton();
    const bool mouse_own = input != nullptr && !input->is_emulating_touch_from_mouse();
    const ::godot::Ref<::godot::InputEventScreenTouch> touch = local;
    const ::godot::Ref<::godot::InputEventMouseButton> mouse = local;
    const ::godot::Ref<::godot::InputEventScreenDrag> drag = local;
    const ::godot::Ref<::godot::InputEventMouseMotion> motion = local;
    int64_t finger = VivariumFigureControls::NO_FINGER;
    ::godot::Vector2 at;
    bool pressed = false;
    bool moved = false;
    if (touch.is_valid()) {
        finger = touch->get_index();
        at = touch->get_position();
        pressed = touch->is_pressed();
    } else if (mouse.is_valid() && mouse_own && mouse->get_button_index() == ::godot::MOUSE_BUTTON_LEFT) {
        finger = VivariumFigureControls::MOUSE_FINGER;
        at = mouse->get_position();
        pressed = mouse->is_pressed();
    } else if (drag.is_valid()) {
        finger = drag->get_index();
        at = drag->get_position();
        moved = true;
    } else if (motion.is_valid() && mouse_own) {
        finger = VivariumFigureControls::MOUSE_FINGER;
        at = motion->get_position();
        moved = true;
    } else {
        return false;
    }
    const ::godot::Vector2 point = hud_frame_.affine_inverse().xform(at);  // the mockup's units
    const ::godot::String figure = ::godot::String::num_uint64(controls.led);

    if (moved) {
        if (finger == controls.tap_finger && static_cast<double>(point.distance_to(controls.tap_at)) > controls.tap_slop) {
            controls.tap_finger = VivariumFigureControls::NO_FINGER;  // it wandered: a drag, no tap
        }
        if (finger == controls.stick_finger) {
            // the stick follows the finger and stops at its radius
            ::godot::Vector2 off = point - controls.stick_origin;
            const float reach = static_cast<float>(controls.stick_radius);
            const float length = off.length();
            if (length > reach) {
                off *= reach / length;
            }
            controls.stick_at = controls.stick_origin + off;
            return true;
        }
        if (finger == controls.turn_finger) {
            // the arc turns the standing figure by the finger's sideways way, as turnFig += dx * yaw_per_unit
            const double yaw_delta = static_cast<double>(point.x - controls.turn_last_x) * controls.turn.yaw_per_unit();
            controls.turn_last_x = point.x;
            if (yaw_delta != 0.0) {
                const ase::kernel::HostStatus turned = resources_.turn_figure(controls.led, yaw_delta);
                if (turned != ase::kernel::HostStatusOk) {
                    note("figure turn figure=" + figure + " status " + ::godot::String::num_int64(turned));
                }
            }
            return true;
        }
        if (finger == controls.zoom_finger) {
            // the rail: the metres between the finger's last height and this one, as camDist = zoomAt(y)
            const double zoom_delta = controls.zoom.distance_at(static_cast<double>(point.y)) -
                                      controls.zoom.distance_at(static_cast<double>(controls.zoom_last_y));
            controls.zoom_last_y = point.y;
            if (zoom_delta != 0.0) {
                const ase::kernel::HostStatus zoomed = resources_.zoom_figure_camera(controls.led, zoom_delta);
                if (zoomed != ase::kernel::HostStatusOk) {
                    note("figure zoom figure=" + figure + " status " + ::godot::String::num_int64(zoomed));
                }
            }
            return true;
        }
        if (finger == controls.orbit_finger || finger == controls.pinch_finger) {
            (finger == controls.orbit_finger ? controls.orbit_at : controls.pinch_at) = point;
            if (controls.pinch_finger != VivariumFigureControls::NO_FINGER) {
                // two fingers on the orbit half pinch the distance and turn nothing
                const double zoom_delta = pinch_delta(controls);
                if (zoom_delta != 0.0) {
                    const ase::kernel::HostStatus zoomed = resources_.zoom_figure_camera(controls.led, zoom_delta);
                    if (zoomed != ase::kernel::HostStatusOk) {
                        note("figure pinch figure=" + figure + " status " + ::godot::String::num_int64(zoomed));
                    }
                }
                return true;
            }
            if (controls.orbit_still) {
                return true;  // the finger a pinch left behind turns nothing, as the mockup's orbit stays off
            }
            // the drag turns the camera by its horizontal way only, as camYawT -= dx * yaw_per_unit
            const double yaw_delta =
                -static_cast<double>(point.x - controls.orbit_last_x) * controls.orbit_yaw_per_unit;
            controls.orbit_last_x = point.x;
            if (yaw_delta != 0.0) {
                const ase::kernel::HostStatus turned = resources_.turn_figure_camera(controls.led, yaw_delta, 0.0);
                if (turned != ase::kernel::HostStatusOk) {
                    note("figure orbit figure=" + figure + " status " + ::godot::String::num_int64(turned));
                }
            }
            return true;
        }
        return false;
    }

    if (!pressed) {
        bool taken = false;
        if (controls.pinch_finger != VivariumFigureControls::NO_FINGER &&
            (finger == controls.orbit_finger || finger == controls.pinch_finger)) {
            // one of the two lifts: the pinch ends, the other stays on the orbit half and turns nothing
            if (finger == controls.orbit_finger) {
                controls.orbit_finger = controls.pinch_finger;
                controls.orbit_at = controls.pinch_at;
            }
            controls.pinch_finger = VivariumFigureControls::NO_FINGER;
            controls.orbit_still = true;
            taken = true;
        }
        for (int64_t* held : {&controls.stick_finger, &controls.orbit_finger, &controls.turn_finger,
                              &controls.zoom_finger}) {
            if (finger == *held) {
                *held = VivariumFigureControls::NO_FINGER;  // the stick: the next frame sends it at rest once
                taken = true;
            }
        }
        if (finger != controls.tap_finger) {
            return taken;
        }
        controls.tap_finger = VivariumFigureControls::NO_FINGER;
        if (static_cast<double>(point.distance_to(controls.tap_at)) > controls.tap_slop ||
            hud_clock_ - controls.tap_since > controls.tap_s) {
            return taken;  // too far or too long: a hold, a stick or an orbit, never a tap
        }
        if (controls.cam_switch.hit(point)) {
            // cam.mode: the fixed view and the player camera, both ways, one switch per tap
            const ase::kernel::HostStatus switched = resources_.switch_figure_camera(controls.led);
            note("figure switch figure=" + figure + " status " + ::godot::String::num_int64(switched));
            return true;
        }
        // THE HUD OVER THE WORLD KEEPS ITS TAP (B0368). In the fixed view the press went to the HUD (below), so a
        // release over a block the HUD draws above the world - a fan chip, a sign, the hand - finishes there and never
        // opens the figure's focus underneath: the water chip at 268,820 lay over the figure and its take never came.
        // Only where the HUD hits the world itself (ground, tree, trunk, a thing or the place on the ground) or
        // nothing at all do the comb, the figure and its arc answer the tap.
        if (hud_live()) {
            const VivariumHudHit over = hud_.hit(point, hud_now_);
            const bool world = over.act == nullptr || over.act == VivariumHud::ACT_GROUND ||
                               over.act == VivariumHud::ACT_TREE || over.act == VivariumHud::ACT_TRUNK ||
                               (over.from != nullptr && over.from == GODOT_HUD_PLACE_GROUND);
            if (!world) {
                return taken;
            }
        }
        if (controls.focus_open && controls.focus.hit_comb(point)) {
            // soul.mode: the port names the direction, the view asks for the class the figure does not have (07.11)
            const ase::kernel::HostStatus handed =
                controls.led_self ? resources_.release_figure(controls.led) : resources_.seize_figure(controls.led);
            note(::godot::String("figure soul figure=") + figure + (controls.led_self ? " release" : " seize") +
                 " status " + ::godot::String::num_int64(handed));
            return true;
        }
        if (controls.focus.hit_figure(point) || controls.turn.hit(point)) {
            // the figure or its arc: the focus opens, and the next tap on either closes it
            controls.focus_open = !controls.focus_open;
            if (controls.focus_open) {
                controls.focus_since = hud_clock_;
            } else {
                controls.focus.close();
            }
            return true;
        }
        if (controls.focus_open) {
            controls.focus_open = false;  // a tap beside the figure closes its focus and stays the HUD's
            controls.focus.close();
        }
        return taken;
    }

    // a press: every finger may still become a tap until it wanders or stays
    controls.tap_finger = finger;
    controls.tap_at = point;
    controls.tap_since = hud_clock_;
    if (!figure_view_shown_) {
        return false;  // the fixed view keeps every other touch for the HUD; only the tap goes on
    }
    if (controls.zoom_finger == VivariumFigureControls::NO_FINGER && controls.zoom.hit(point)) {
        // the rail: the camera's distance jumps to the finger's height, then follows it
        controls.tap_finger = VivariumFigureControls::NO_FINGER;
        controls.zoom_finger = finger;
        controls.zoom_last_y = point.y;
        if (controls.camera_drawn) {
            const double zoom_delta = controls.zoom.distance_at(static_cast<double>(point.y)) - controls.camera_metres;
            const ase::kernel::HostStatus zoomed = resources_.zoom_figure_camera(controls.led, zoom_delta);
            if (zoomed != ase::kernel::HostStatusOk) {
                note("figure zoom figure=" + figure + " status " + ::godot::String::num_int64(zoomed));
            }
        }
        return true;
    }
    if (controls.turn_finger == VivariumFigureControls::NO_FINGER && controls.turn.hit(point)) {
        if (!controls.led_self) {
            // a finger on the arc takes the figure back from the AI first: the turn port turns only a led figure
            const ase::kernel::HostStatus seized = resources_.seize_figure(controls.led);
            note("figure soul figure=" + figure + " seize by turn status " + ::godot::String::num_int64(seized));
        }
        controls.turn_finger = finger;
        controls.turn_last_x = point.x;
        return true;
    }
    if (controls.stick_finger == VivariumFigureControls::NO_FINGER && controls.stick_zone.has_point(point)) {
        if (!controls.led_self) {
            // a thumb on the stick takes the figure back from the AI first, as the mockup's zoneStick does
            const ase::kernel::HostStatus seized = resources_.seize_figure(controls.led);
            note("figure soul figure=" + figure + " seize by stick status " + ::godot::String::num_int64(seized));
        }
        controls.stick_finger = finger;
        controls.stick_origin = point;
        controls.stick_at = point;
        return true;
    }
    if (controls.orbit_finger == VivariumFigureControls::NO_FINGER && controls.orbit_zone.has_point(point)) {
        controls.orbit_finger = finger;
        controls.orbit_last_x = point.x;
        controls.orbit_at = point;
        controls.orbit_still = false;
        return true;
    }
    if (controls.pinch_finger == VivariumFigureControls::NO_FINGER && controls.camera_drawn &&
        controls.orbit_zone.has_point(point)) {
        // a second finger beside the orbit's: the two pinch the distance from the one the rail drew last
        controls.tap_finger = VivariumFigureControls::NO_FINGER;
        controls.pinch_finger = finger;
        controls.pinch_at = point;
        controls.pinch_spread = static_cast<double>(controls.orbit_at.distance_to(point));
        controls.pinch_from = controls.camera_metres;
        controls.pinch_asked = controls.camera_metres;
        return true;
    }
    return false;
}

}  // namespace ase::adp::godot
