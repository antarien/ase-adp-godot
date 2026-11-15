/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW FIGURE
 *
 * @file        godot_vivarium_figure.cpp
 * @brief       VivariumView - the figure of the seat: its configuration, its body in the scene and the input that leads it
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.4 to 07.9: the start configuration names the six ports of
 *              ase-pl-avatar, the four operations the view submits through them and the seat and place the
 *              figure is spawned at; GodotHostResourceManager::boot asks for it once the host stands.
 *              follow_figure runs every frame after follow_scene: it lays every figure of the body port into
 *              the scene (godot_figure_scene.hpp), stands the scene's camera at the player camera's eye while
 *              the camera port carries one, and hands the fixed view of Phase 06 its camera back the frame the
 *              port carries none. The keys of the desktop lead the figure of this seat: W A S D the stick (07.4),
 *              Shift and Space sprint and jump, the arrow keys turn the player camera (07.5), C is cam.mode,
 *              Q and E turn the figure itself on the spot (fig.turn, 07.7). Once per figure node the rig the
 *              scene measured (eye height and height, godot_figure_rig.hpp) goes to the rig port (07.6).
 *              A thumb on the phone's stick (godot_vivarium_figure_controls.cpp, 07.9) leads over the keys: its
 *              deflection is sent through the same stick port in their place, running from run_share on.
 *              Nothing about the figure is computed here; ase-player moves it, ase-camera places its camera.
 *              The hand of Phase 06 (07.10): whenever the thing in the hand changes, a hold request with its
 *              catalog id or a drop request goes out for the figure led; ase-equipment puts it on at
 *              socket_hand_r, and the gear port brings the piece back for the scene to hang under the socket.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_021
 * @category    ecs/module
 * @usecase     usecase:101554 - receive player input
 * @actor       Player Input Warden (PLAYINPWAR)
 * @created     2026-11-13
 * @modified    2026-11-15
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_figure_scene.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/kernel/kernel_embedded_host.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input.hpp>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

namespace {

/**
 * THE KEYS OF THE DESKTOP. The phone leads the figure through the stick and the drag of the HUD (07.4,
 * 07.5); on the desktop the keys stand in for both. Their rates are SHA's hud_figure_keys_*_rate, read with the
 * HUD into VivariumFigureControls (load_figure_hud); only the level of a pressed key stands here.
 */
constexpr double kFigureAxisFull = 1.0;          // a pressed key moves its stick axis to the end of its range
constexpr double kFigureLevelOn = 1.0;           // sprint and jump: the level of a pressed key
constexpr double kFigureSeatMax = 4294967295.0;  // the largest seat a uint32_t holds
constexpr double kFigurePlaceStepM = 0.25;       // metres the led figure moves before its place line is written again

/** The level of one key: kFigureAxisFull while it is held, else 0. */
double held(const ::godot::Input* input, ::godot::Key key) {
    return input->is_key_pressed(key) ? kFigureAxisFull : 0.0;
}

}  // namespace

bool VivariumView::load_figure_config(const ::godot::Dictionary& root) {
    // THE FIGURE OF THE SEAT (07.8): the ports of ase-pl-avatar, the operations through them and where the
    // figure of this view's seat is spawned. Every key is required; the manager names the ports it misses
    // once the host stands, so a unit set without the plugin boots and draws no figure.
    ::godot::String text;
    ::godot::Dictionary ports;
    if (!read_section(root, "ports", ports)) {
        return fail_config("ports");
    }
    if (!read_text(ports, "figure_body", text)) {
        return fail_config("ports.figure_body");
    }
    start_.figure_body_port = utf8_of(text);
    if (!read_text(ports, "figure_camera", text)) {
        return fail_config("ports.figure_camera");
    }
    start_.figure_camera_port = utf8_of(text);
    if (!read_text(ports, "figure_spawn", text)) {
        return fail_config("ports.figure_spawn");
    }
    start_.figure_spawn_port = utf8_of(text);
    if (!read_text(ports, "figure_stick", text)) {
        return fail_config("ports.figure_stick");
    }
    start_.figure_stick_port = utf8_of(text);
    if (!read_text(ports, "figure_orbit", text)) {
        return fail_config("ports.figure_orbit");
    }
    start_.figure_orbit_port = utf8_of(text);
    if (!read_text(ports, "figure_switch", text)) {
        return fail_config("ports.figure_switch");
    }
    start_.figure_switch_port = utf8_of(text);
    if (!read_text(ports, "figure_release", text)) {
        return fail_config("ports.figure_release");
    }
    start_.figure_release_port = utf8_of(text);
    if (!read_text(ports, "figure_seize", text)) {
        return fail_config("ports.figure_seize");
    }
    start_.figure_seize_port = utf8_of(text);
    if (!read_text(ports, "figure_hold", text)) {
        return fail_config("ports.figure_hold");
    }
    start_.figure_hold_port = utf8_of(text);
    if (!read_text(ports, "figure_drop", text)) {
        return fail_config("ports.figure_drop");
    }
    start_.figure_drop_port = utf8_of(text);
    if (!read_text(ports, "figure_gear", text)) {
        return fail_config("ports.figure_gear");
    }
    start_.figure_gear_port = utf8_of(text);
    if (!read_text(ports, "figure_code", text)) {
        return fail_config("ports.figure_code");
    }
    start_.figure_code_port = utf8_of(text);
    if (!read_text(ports, "figure_rig", text)) {
        return fail_config("ports.figure_rig");
    }
    start_.figure_rig_port = utf8_of(text);
    if (!read_text(ports, "figure_turn", text)) {
        return fail_config("ports.figure_turn");
    }
    start_.figure_turn_port = utf8_of(text);

    ::godot::Dictionary operations;
    if (!read_section(root, "operations", operations)) {
        return fail_config("operations");
    }
    if (!read_text(operations, "spawn", text)) {
        return fail_config("operations.spawn");
    }
    start_.op_spawn = utf8_of(text);
    if (!read_text(operations, "steer", text)) {
        return fail_config("operations.steer");
    }
    start_.op_steer = utf8_of(text);
    if (!read_text(operations, "turn", text)) {
        return fail_config("operations.turn");
    }
    start_.op_turn = utf8_of(text);
    if (!read_text(operations, "switch", text)) {
        return fail_config("operations.switch");
    }
    start_.op_switch = utf8_of(text);
    if (!read_text(operations, "release", text)) {
        return fail_config("operations.release");
    }
    start_.op_release = utf8_of(text);
    if (!read_text(operations, "seize", text)) {
        return fail_config("operations.seize");
    }
    start_.op_seize = utf8_of(text);
    if (!read_text(operations, "hold", text)) {
        return fail_config("operations.hold");
    }
    start_.op_hold = utf8_of(text);
    if (!read_text(operations, "drop", text)) {
        return fail_config("operations.drop");
    }
    start_.op_drop = utf8_of(text);
    if (!read_text(operations, "measure", text)) {
        return fail_config("operations.measure");
    }
    start_.op_measure = utf8_of(text);

    ::godot::Dictionary figure;
    if (!read_section(root, "figure", figure)) {
        return fail_config("figure");
    }
    double seat = 0.0;
    if (!read_number(figure, "seat", seat) || !(seat >= 0.0) || seat > kFigureSeatMax ||
        static_cast<double>(static_cast<uint32_t>(seat)) != seat) {
        return fail_config("figure.seat");
    }
    start_.figure_seat = static_cast<uint32_t>(seat);
    if (!read_number(figure, "x", start_.figure_x) || !std::isfinite(start_.figure_x)) {
        return fail_config("figure.x");
    }
    if (!read_number(figure, "z", start_.figure_z) || !std::isfinite(start_.figure_z)) {
        return fail_config("figure.z");
    }
    return true;
}

void VivariumView::follow_figure(double elapsed_s) {
    if (scene_ == nullptr) {
        return;
    }
    // the frame the task of the figure file completed (godot_figure_load.hpp): its pack becomes the model, and
    // one instance warms its pipelines behind the start screen, which held while the file loaded
    if (take_figure_load(figure_look_.load, figure_look_.model)) {
        warm_figure(scene_, figure_look_);
        note(::godot::String("figure file ") + (figure_look_.model.is_valid() ? "loaded " : "not loaded ") +
             text_of(figure_look_.file) + " " + figure_load_times(figure_look_.load));
    }
    // 07.10: the frame a tool's task completed, its pack becomes the tool's model; carry_tool redraws the hand
    for (auto& [kind, tool] : figure_look_.tools) {
        if (take_figure_load(tool.load, tool.model)) {
            note(::godot::String("tool file ") + (tool.model.is_valid() ? "loaded " : "not loaded ") +
                 text_of(tool.file) + " " + figure_load_times(tool.load));
        }
    }
    // the body of the tool place: the frame its task completed, lay_tools redraws it from the file
    if (take_figure_load(figure_look_.rack.load, figure_look_.rack.model)) {
        note(::godot::String("rack file ") + (figure_look_.rack.model.is_valid() ? "loaded " : "not loaded ") +
             text_of(figure_look_.rack.file) + " " + figure_load_times(figure_look_.rack.load));
    }
    const std::vector<VivariumFigureView>& figures = resources_.get_figures();
    const VivariumFigureCodeView& codes = resources_.get_figure_code();
    fill_figures(scene_, figures, resources_.get_figure_gear(), figure_look_, codes);

    // the rig of every figure node, once per node (07.6): eye height and height of the skeleton the scene built go
    // to the rig port, so the face landing of ase-camera aims at the eyes of this file and not at a guessed height
    for (const VivariumFigureView& figure : figures) {
        double eye_height = 0.0;
        double height = 0.0;
        if (take_figure_rig(scene_, figure.object_id, eye_height, height)) {
            const ase::kernel::HostStatus measured = resources_.measure_figure(figure.object_id, eye_height, height);
            note(::godot::String("figure rig figure=") + ::godot::String::num_uint64(figure.object_id) + " eye " +
                 ::godot::String::num(eye_height, 3) + " height " + ::godot::String::num(height, 3) + " status " +
                 ::godot::String::num_int64(measured));
        }
    }

    // the player camera (07.6): while the camera port carries one, the scene's camera stands at its eye;
    // the frame it carries none any more, the fixed view of Phase 06 aims its camera again (follow_scene)
    const VivariumFigureCameraView& camera = resources_.get_figure_camera();
    const bool shown = aim_figure_camera(scene_, camera);
    if (shown != figure_view_shown_) {
        if (!shown) {
            scene_focus_ = GODOT_NO_PLACE;
            figure_controls_.stick_finger = VivariumFigureControls::NO_FINGER;  // the fixed view takes no stick
            figure_controls_.orbit_finger = VivariumFigureControls::NO_FINGER;
        }
        note(::godot::String("figure camera ") + (shown ? "player" : "fixed") + " figure=" +
             ::godot::String::num_uint64(camera.figure) + " eye " + ::godot::String::num(camera.eye_x, 2) + " " +
             ::godot::String::num(camera.eye_y, 2) + " " + ::godot::String::num(camera.eye_z, 2));
        figure_view_shown_ = shown;
    }

    // the figure of this seat: the one it held last frame while that still stands, else the first of this frame
    // whose soul is SS/PC. A figure handed to the AI (soul.mode, 07.11) stays the seat's, so cam.mode and
    // soul.mode keep answering and the player can take it back.
    VivariumFigureControls& controls = figure_controls_;
    const VivariumFigureView* led = nullptr;
    const VivariumFigureView* first_self = nullptr;
    for (const VivariumFigureView& figure : figures) {
        if (controls.led_known && figure.object_id == controls.led) {
            led = &figure;
        }
        if (first_self == nullptr && codes.known && figure.soul == codes.soul_player) {
            first_self = &figure;
        }
    }
    if (led == nullptr) {
        led = first_self;
    }
    controls.led_known = led != nullptr;  // cam.mode and soul.mode answer only while a figure of this seat stands
    controls.led = led != nullptr ? led->object_id : 0;
    controls.led_self = led != nullptr && codes.known && led->soul == codes.soul_player;
    if (led == nullptr) {
        controls.stick_finger = VivariumFigureControls::NO_FINGER;
        controls.orbit_finger = VivariumFigureControls::NO_FINGER;
    }
    if (led == nullptr) {
        figure_hand_sent_ = false;  // a figure led anew is told the hand's state again
    }

    // the place of the figure led, at its first stand and then per kFigurePlaceStepM: the line the acceptance reads
    // the walk from, in the camera line's frame (x east, y up, z north)
    double place_x = 0.0;
    double place_y = 0.0;
    double place_z = 0.0;
    if (led != nullptr && take_figure_place(scene_, led->object_id, kFigurePlaceStepM, place_x, place_y, place_z)) {
        note(::godot::String("figure place figure=") + ::godot::String::num_uint64(led->object_id) + " x " +
             ::godot::String::num(place_x, 2) + " y " + ::godot::String::num(place_y, 2) + " z " +
             ::godot::String::num(place_z, 2));
    }

    // the hand of Phase 06 (07.10): what it holds the figure carries at socket_hand_r. One request per change of
    // the thing in the hand or of the figure led; the item travels as the catalog id of its key, which ase-pl-avatar
    // files as HUB_EQP_SLOT_REQ and ase-equipment puts on. The gear port shows the result (fill_figures).
    if (led != nullptr) {
        const HandThing* thing = hud_now_.hand.thing;
        const uint32_t item = thing != nullptr ? ase::kernel::KernelEmbeddedHost::name_id(thing->id.c_str()) : 0u;
        if (!figure_hand_sent_ || figure_hand_figure_ != led->object_id || figure_hand_item_ != item) {
            const ase::kernel::HostStatus handed =
                item != 0u ? resources_.hold_figure(led->object_id, item) : resources_.drop_figure(led->object_id);
            note(::godot::String("figure ") + (item != 0u ? "hold" : "drop") + " figure=" +
                 ::godot::String::num_uint64(led->object_id) + " item=" +
                 ::godot::String::utf8(thing != nullptr ? thing->id.c_str() : "") + " status " +
                 ::godot::String::num_int64(handed));
            figure_hand_sent_ = true;  // a refused request is noted once; the next change of the hand asks again
            figure_hand_figure_ = led->object_id;
            figure_hand_item_ = item;
        }
    }

    const ::godot::Input* input = ::godot::Input::get_singleton();
    if (led == nullptr || input == nullptr) {
        return;
    }

    // cam.mode (07.5): one switch per press of C, on its rising edge
    const bool switch_held = input->is_key_pressed(::godot::KEY_C);
    if (switch_held && !figure_switch_held_) {
        const ase::kernel::HostStatus switched = resources_.switch_figure_camera(led->object_id);
        note("figure switch figure=" + ::godot::String::num_uint64(led->object_id) + " status " +
             ::godot::String::num_int64(switched));
    }
    figure_switch_held_ = switch_held;

    // the stick (07.4): forward and sideways from W A S D; the course they walk along is the course of the
    // active camera, read by ase-pl-avatar from ase-camera. A released stick is sent once more with both axes
    // at 0, so the figure stops
    double forward = held(input, ::godot::KEY_W) - held(input, ::godot::KEY_S);
    double strafe = held(input, ::godot::KEY_D) - held(input, ::godot::KEY_A);  // negative is left (PlayerStaInpMovComponent)
    double sprint = input->is_key_pressed(::godot::KEY_SHIFT) ? kFigureLevelOn : 0.0;
    const double jump = input->is_key_pressed(::godot::KEY_SPACE) ? kFigureLevelOn : 0.0;
    if (controls.stick_finger != VivariumFigureControls::NO_FINGER) {
        // the thumb leads over the keys (07.9): full walking pace at walk_share of the radius, running from
        // run_share on; up on the screen is forward
        const ::godot::Vector2 off = controls.stick_at - controls.stick_origin;
        const double share = static_cast<double>(off.length()) / controls.stick_radius;
        const double gain = share > 0.0 ? ase::math::min(kFigureAxisFull, share / controls.walk_share) / share : 0.0;
        forward = -static_cast<double>(off.y) / controls.stick_radius * gain;
        strafe = static_cast<double>(off.x) / controls.stick_radius * gain;
        sprint = share >= controls.run_share ? kFigureLevelOn : 0.0;
    }
    // only a figure its player leads takes the stick; under AI/PC the AI steers it (07.11)
    const bool steering = controls.led_self && (forward != 0.0 || strafe != 0.0 || jump != 0.0);
    if (controls.led_self && (steering || figure_steered_)) {
        const ase::kernel::HostStatus steered =
            resources_.steer_figure(led->object_id, forward, strafe, sprint, jump);
        if (steered != ase::kernel::HostStatusOk) {
            note("figure steer figure=" + ::godot::String::num_uint64(led->object_id) + " status " +
                 ::godot::String::num_int64(steered));
        }
    }
    figure_steered_ = steering;

    // fig.turn (07.7): Q and E turn the figure itself on the spot; only a figure its player leads takes it
    const double spin =
        (held(input, ::godot::KEY_Q) - held(input, ::godot::KEY_E)) * controls.key_spin_rate * elapsed_s;
    if (controls.led_self && spin != 0.0) {
        const ase::kernel::HostStatus spun = resources_.turn_figure(led->object_id, spin);
        if (spun != ase::kernel::HostStatusOk) {
            note("figure spin figure=" + ::godot::String::num_uint64(led->object_id) + " status " +
                 ::godot::String::num_int64(spun));
        }
    }

    // the drag of the right half (07.5): the arrow keys turn the player camera around the figure and tilt it
    const double yaw_delta =
        (held(input, ::godot::KEY_LEFT) - held(input, ::godot::KEY_RIGHT)) * controls.key_turn_rate * elapsed_s;
    const double pitch_delta =
        (held(input, ::godot::KEY_UP) - held(input, ::godot::KEY_DOWN)) * controls.key_pitch_rate * elapsed_s;
    if (yaw_delta != 0.0 || pitch_delta != 0.0) {
        const ase::kernel::HostStatus turned = resources_.turn_figure_camera(led->object_id, yaw_delta, pitch_delta);
        if (turned != ase::kernel::HostStatusOk) {
            note("figure turn figure=" + ::godot::String::num_uint64(led->object_id) + " status " +
                 ::godot::String::num_int64(turned));
        }
    }

    // cam.zoom (07.4, 07.6): Z draws the player camera towards the figure, down to its face, X back out; the
    // rail and its ends are ase-camera's, the touch rail of the HUD is ase-ui's (godot_hud_figure_zoom)
    const double zoom_delta =
        (held(input, ::godot::KEY_X) - held(input, ::godot::KEY_Z)) * controls.key_zoom_rate * elapsed_s;
    if (zoom_delta != 0.0) {
        const ase::kernel::HostStatus zoomed = resources_.zoom_figure_camera(led->object_id, zoom_delta);
        if (zoomed != ase::kernel::HostStatusOk) {
            note("figure zoom figure=" + ::godot::String::num_uint64(led->object_id) + " status " +
                 ::godot::String::num_int64(zoomed));
        }
    }
    // the frame the zoom keys are let go, the eye and the aim the rail settled at are noted once: the distance
    // between them is the zoom step a run judges (vivarium_figure_acceptance.py, face still). Face and eye as they
    // are drawn ride on the same line (07.3): the coarser detail level shown and the edges of the albedo on the face
    // and the iris material, which a run holds against level 0 and the edges of the source images; "face none"
    // while no face and eye are drawn
    if (zoom_delta == 0.0 && figure_zoomed_) {
        int32_t face_level = 0;
        int64_t face_edge = 0;
        int64_t eye_edge = 0;
        const ::godot::String face =
            figure_face_look(scene_, camera.figure, face_level, face_edge, eye_edge)
                ? " lod " + ::godot::String::num_int64(face_level) + " face " +
                      ::godot::String::num_int64(face_edge) + " eye " + ::godot::String::num_int64(eye_edge)
                : ::godot::String(" face none");
        note("figure zoomed figure=" + ::godot::String::num_uint64(camera.figure) + " eye " +
             ::godot::String::num(camera.eye_x, 2) + " " + ::godot::String::num(camera.eye_y, 2) + " " +
             ::godot::String::num(camera.eye_z, 2) + " aim " + ::godot::String::num(camera.aim_x, 2) + " " +
             ::godot::String::num(camera.aim_y, 2) + " " + ::godot::String::num(camera.aim_z, 2) + face);
    }
    figure_zoomed_ = zoom_delta != 0.0;
}

}  // namespace ase::adp::godot
