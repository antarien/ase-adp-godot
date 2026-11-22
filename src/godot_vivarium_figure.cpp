/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW FIGURE
 *
 * @file        godot_vivarium_figure.cpp
 * @brief       VivariumView::follow_figure - the figure of the seat: its body in the scene and the input that leads it
 * @description PLAN_ASE_VIVARIUM_PHASE_07_FIGURE 07.4 to 07.9: follow_figure runs every frame after follow_scene: it
 *              lays every figure of the body port into the scene (godot_figure_scene.hpp), stands the scene's camera
 *              at the player camera's eye while the camera port carries one, and hands the fixed view of Phase 06 its
 *              camera back the frame the port carries none. The keys of the desktop lead the figure of this seat:
 *              W A S D the stick (07.4), Shift and Space sprint and jump, the arrow keys turn the player camera
 *              (07.5), C is cam.mode, Q and E turn the figure itself on the spot (fig.turn, 07.7). Once per figure
 *              node the rig the scene measured (eye height and height, godot_figure_rig.hpp) goes to the rig port
 *              (07.6). A thumb on the phone's stick (godot_vivarium_figure_controls.cpp, 07.9) leads over the keys:
 *              its deflection is sent through the same stick port in their place, running from run_share on.
 *              Nothing about the figure is computed here; ase-player moves it, ase-camera places its camera.
 *              The scene gets the rest port and the places of the ring stones as ase-physics holds them, so a
 *              resting figure sits on a stone or the ground and a sleeping one lies (PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP
 *              T.11); ase-metabolism says whether it rests and sleeps.
 *              The hand of Phase 06 (07.10): whenever the thing in the hand changes, a hold request with its
 *              catalog id or a drop request goes out for the figure led; ase-equipment puts it on at
 *              socket_hand_r, and the gear port brings the piece back for the scene to hang under the socket.
 *              The two rims of 07-G are written as lines on each edge: the figure held at a chunk border of the
 *              loaded raster (figure edge, RAND DES AREALS) and the player camera held on it (figure bound, cam.bound).
 *              The start configuration the view reads once stands in godot_vivarium_figure_config.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_021
 * @category    ecs/module
 * @usecase     usecase:101554 - receive player input
 * @actor       Player Input Warden (PLAYINPWAR)
 * @created     2026-11-13
 * @modified    2026-11-20
 * @version     00.00.26.00026 [refine]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_figure_scene.hpp>
#include <ase/adp/godot/godot_view_support.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/kernel/kernel_embedded_host.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/input.hpp>

#include <algorithm>
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
constexpr double kFigurePlaceStepM = 0.25;       // metres the led figure moves before its place line is written again
constexpr const char* kFigureCampStoneRole = "camp.stone";  // the role of the ring stones in camp.parts (T.3, T.11)

/** true when `make` is one of the makes the export of the ring stones carries. */
bool stone_make(const CampRoleLoad& stones, uint32_t make) {
    for (const CampMake& candidate : stones.makes) {
        if (candidate.make == make) {
            return true;
        }
    }
    return false;
}

/** The level of one key: kFigureAxisFull while it is held, else 0. */
double held(const ::godot::Input* input, ::godot::Key key) {
    return input->is_key_pressed(key) ? kFigureAxisFull : 0.0;
}

}  // namespace

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
    // the ring stones of the camp fire where ase-physics holds their bodies (T.3), in the frame of the scene as the
    // camp view draws them; a resting figure over one of them sits on it (T.11)
    std::vector<FigureSceneStone> stones;
    const auto stone_load = camp_.loads.find(kFigureCampStoneRole);
    if (stone_load != camp_.loads.end()) {
        for (const VivariumCampBodyView& body : resources_.get_camp_bodies()) {
            const VivariumPatchView* frame = resources_.get_patch(body.patch);
            if (frame != nullptr && stone_make(stone_load->second, body.make)) {
                stones.push_back(FigureSceneStone{frame->x + body.x, frame->z + body.z});
            }
        }
    }
    fill_figures(scene_, figures, resources_.get_figure_gear(), figure_look_, codes, resources_.get_survival_rests(),
                 stones);

    // the rig of every figure node, once per node (07.6): eye height, height and body radius of the skeleton the
    // scene built go to the rig port, so the face landing of ase-camera aims at the eyes of this file and the capsule
    // ase-physics collides the walking body as has the measure of this file, not a guessed one
    for (const VivariumFigureView& figure : figures) {
        double eye_height = 0.0;
        double height = 0.0;
        double radius = 0.0;
        if (take_figure_rig(scene_, figure.object_id, eye_height, height, radius)) {
            const ase::kernel::HostStatus measured =
                resources_.measure_figure(figure.object_id, eye_height, height, radius);
            note(::godot::String("figure rig figure=") + ::godot::String::num_uint64(figure.object_id) + " eye " +
                 ::godot::String::num(eye_height, 3) + " height " + ::godot::String::num(height, 3) + " radius " +
                 ::godot::String::num(radius, 3) + " status " + ::godot::String::num_int64(measured));
        }
    }

    // the area of every figure, once per figure (07.7, usecase:101612 keep figure in loaded area): the plate is a
    // relation the start config names (figure.place), never found by where the figure stands (B0447). The view hands
    // only the object id of that patch's plate on; ase-player bounds the figure to the four chunk borders of the
    // loaded raster around it (PlayerSimBndySystem, HUB_GIS_GRND) and marks its rim, which the camera record carries
    // back as edge. Until the plate stands in the scene the figure waits unbound.
    const VivariumPatchView* plate = resources_.get_patch_at(start_.figure_place);
    for (const VivariumFigureView& figure : figures) {
        if (plate == nullptr) {
            break;
        }
        if (std::find(figure_area_bound_.begin(), figure_area_bound_.end(), figure.object_id) !=
            figure_area_bound_.end()) {
            continue;
        }
        const ase::kernel::HostStatus bound = resources_.bind_figure_area(figure.object_id, plate->object_id);
        note(::godot::String("figure area figure=") + ::godot::String::num_uint64(figure.object_id) + " plate " +
             ::godot::String::num_uint64(plate->object_id) + " place " +
             ::godot::String::num_uint64(start_.figure_place) + " status " + ::godot::String::num_int64(bound));
        if (bound == ase::kernel::HostStatusOk) {
            figure_area_bound_.push_back(figure.object_id);
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

    // the rim of the area (07-G, RAND DES AREALS): ase-physics holds the followed figure at a chunk border of the
    // loaded ground (HUB_PHYS_STAND_BNDY, 07.13) and the camera record carries that mark; the line is written on each
    // edge, never per frame. The fixed view follows no figure and stands on no rim.
    const bool edge = shown && camera.edge;
    if (edge != figure_edge_shown_) {
        note(::godot::String("figure edge figure=") + ::godot::String::num_uint64(camera.figure) + " value " +
             (edge ? "1" : "0"));
        figure_edge_shown_ = edge;
    }

    // the rim of the camera (07-G, cam.bound): ase-camera holds the player camera at the chunk borders of the loaded raster
    // (HUB_CAM_OWNR_BNDY, usecase:101612 keep figure in loaded area) and ase-pl-avatar hands that mark over as the
    // key bound of the camera record; the line is written on each edge, never per frame. The fixed view of
    // Phase 06 is no player camera and stands on no rim.
    const bool bound = shown && camera.bound;
    if (bound != figure_bound_shown_) {
        note(::godot::String("figure bound figure=") + ::godot::String::num_uint64(camera.figure) + " value " +
             (bound ? "1" : "0"));
        figure_bound_shown_ = bound;
    }

    // 07.13 and M0157: the red glass pane where a walking body bumped the chunk edge, from the boundary port, no
    // message with it; it stands at full glow for as long as the rim holds the followed figure (edge or bound above)
    flash_figure_edges(scene_, resources_.get_figure_bumps(), figures, figure_glass_, camera.figure,
                       figure_edge_shown_ || figure_bound_shown_, elapsed_s);

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

    // the gait of the figure led, at its first stand and then per change (07-F): the value of the body port beside
    // the three codes of the code port, so the acceptance names stand, walk and run from the log and never guesses
    if (led != nullptr && codes.known && take_figure_gait(scene_, led->object_id, led->gait)) {
        note(::godot::String("figure gait figure=") + ::godot::String::num_uint64(led->object_id) + " value " +
             ::godot::String::num(led->gait, 3) + " stand " + ::godot::String::num(codes.gait_stand, 3) + " walk " +
             ::godot::String::num(codes.gait_walk, 3) + " run " + ::godot::String::num(codes.gait_run, 3));
    }

    // the detail level the figure led is drawn on, at its first frame and then per change (07-G, each detail level):
    // the scene picks it by the figure's share of the frame height against the bands of the body port
    int32_t drawn_level = 0;
    if (led != nullptr && take_figure_level(scene_, led->object_id, drawn_level)) {
        note(::godot::String("figure lod figure=") + ::godot::String::num_uint64(led->object_id) + " level " +
             ::godot::String::num_int64(drawn_level));
    }

    // the water the figure led carries, at the first sight of its vessel and then per change (07.14): ase-equipment
    // fills the can at the brook and empties it at the birch, so a rise is the fill and a fall the pour - under SS/PC
    // by the player's tap and under AI/PC by the acts of the BDI alike; the acceptance reads both from this line
    double carried_litres = 0.0;
    if (led != nullptr && take_figure_water(scene_, led->object_id, carried_litres)) {
        note(::godot::String("figure water figure=") + ::godot::String::num_uint64(led->object_id) + " litres " +
             ::godot::String::num(carried_litres, 3));
    }

    // the start stock of the bag (PLAN_ASE_VIVARIUM_PHASE_08_FIRECAMP T.8, usecase:5378 - regulate basic needs): once
    // per figure of this seat, one stock request per piece of figure.stock. The item travels as the catalog id of its
    // item_art.json key; ase-pl-survival files it as HUB_EQP_FOOD_REQ, ase-equipment lays it into the pouch and
    // ase-entropy spoils it there, and the ring ESTRAGO reads the share back from survival.food.v1. A host without the
    // stock port answers NotFound for each piece once and lays nothing; a stock is laid once, never resent.
    if (led != nullptr &&
        std::find(figure_stocked_.begin(), figure_stocked_.end(), led->object_id) == figure_stocked_.end()) {
        for (const VivariumStockSpec& piece : start_.stock) {
            const uint32_t item = ase::kernel::KernelEmbeddedHost::name_id(piece.item.c_str());
            const ase::kernel::HostStatus stocked = resources_.stock_figure(led->object_id, item, piece.kg);
            note(::godot::String("figure stock figure=") + ::godot::String::num_uint64(led->object_id) + " item=" +
                 ::godot::String::utf8(piece.item.c_str()) + " kg " + ::godot::String::num(piece.kg, 3) + " status " +
                 ::godot::String::num_int64(stocked));
        }
        figure_stocked_.push_back(led->object_id);
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
