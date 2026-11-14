/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD INPUT
 *
 * @file        godot_vivarium_hud_input.cpp
 * @brief       VivariumView - the touches through the HUD, its clock and the requests it sends over the ports
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5: while the HUD is live every press, drag and release goes
 *              through VivariumHud in mockup units (the inverse of hud_frame_), and its clock runs on the real
 *              time of the frames. Each request it fires goes through the port that carries it - the pour, the
 *              cut, the pace, the jumps of the debug layer - the terrain verbs and the planting hole in
 *              godot_vivarium_hud_verb.cpp (usecase:100388) - or, where no port
 *              carries it yet, its register id is logged once on the edge of the bridge (open_gap), never shown.
 *
 *              BACKEND GAPS OF THE REQUESTS (register carrier N0448): N0451 take, equip and sow;
 *              N0452 stow and send; N0456 loosen; N0457 trunk and plug;
 *              N0458 the can filled at the brook; N0464 measure.
 *
 *              THE EDGE LINES OF A TAP name the mode and the hit it answers: "hud sap trunk" for the bore,
 *              "hud plug trunk" for the peg, each "done: " or "refused: " with the bridge's reason; the window
 *              acceptance taps every hit of hud.modes.<mode>.hits a place line names and waits for that line.
 *              Every press writes "hud tap act=<act> index=<sign or tool> mode=<mode> x y" before the HUD takes
 *              it, so a tap that changes no mode, or rests where a sign was meant, is read from the log.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-10
 * @modified    2026-11-15
 * @version     00.00.01.00012
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_figure_hand.hpp>
#include <ase/adp/godot/godot_patch_focus.hpp>
#include <ase/adp/godot/godot_plant_clouds.hpp>
#include <ase/adp/godot/types.hpp>
#include <ase/kernel/kernel_types.hpp>
#include <ase/math/math.hpp>

#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_screen_drag.hpp>
#include <godot_cpp/classes/input_event_screen_touch.hpp>
#include <godot_cpp/classes/viewport.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

void VivariumView::hud_input(const ::godot::Ref<::godot::InputEvent>& local) {
    // ONE FINGER: the first touch or the left mouse button; a drag counts only while that finger is down.
    const ::godot::Ref<::godot::InputEventScreenTouch> touch = local;
    const ::godot::Ref<::godot::InputEventMouseButton> mouse = local;
    const ::godot::Ref<::godot::InputEventScreenDrag> drag = local;
    const ::godot::Ref<::godot::InputEventMouseMotion> motion = local;
    ::godot::Vector2 at;
    bool pressed = false;
    bool moved = false;
    if (touch.is_valid()) {
        if (touch->get_index() != 0) {
            return;
        }
        at = touch->get_position();
        pressed = touch->is_pressed();
    } else if (mouse.is_valid() && mouse->get_button_index() == ::godot::MOUSE_BUTTON_LEFT) {
        at = mouse->get_position();
        pressed = mouse->is_pressed();
    } else if (drag.is_valid() && drag->get_index() == 0 && hud_down_) {
        at = drag->get_position();
        moved = true;
    } else if (motion.is_valid() && hud_down_) {
        at = motion->get_position();
        moved = true;
    } else {
        return;
    }
    node_->get_viewport()->set_input_as_handled();
    const ::godot::Vector2 point = hud_frame_.affine_inverse().xform(at);
    std::string failed;
    std::vector<VivariumHudRequest> requests;
    // pointerup on a held grip clears S.wdrag and returns without render() (m.html 1012): read before the release.
    const bool window_let_go = !moved && !pressed && hud_.holding_window();
    if (moved) {
        hud_finger_ = at;
        // THE DRAG AIMS WHERE THE FINGER IS NOW (B0275): the ground under it before the layer aims, the stow
        // bridge's verdict on the place it aimed at after; a frame between two moves reads both again.
        drag_ground(hud_now_.drag);
        if (!hud_.move(point, hud_now_, failed)) {
            fail_hud(failed);
            return;
        }
        bridge_stow(hud_now_.drag);
    } else if (pressed) {
        hud_down_ = true;
        hud_finger_ = at;
        // THE PRESS NAMES WHAT IT HIT: a tap whose act changes no mode (a rest row of the trail, a drawer body, the
        // empty band) leaves no edge, and a tap that rests where a sign was meant looks like one that never came.
        // D01: the ray of the camera through the finger onto the rack body, read before the HUD asks what it hit
        hud_now_.rack_world.struck =
            hud_now_.rack_world.bodied && rack_struck(scene_, node_->get_viewport()->get_camera_3d(), at);
        const VivariumHudHit hit = hud_.hit(point, hud_now_);
        const std::string tap = "hud tap act=" + std::string(hit.act != nullptr ? hit.act : "none") +
                                (hit.index != UINT32_MAX ? " index=" + std::to_string(hit.index) : std::string()) +
                                " mode=" + hud_.mode() + " x=" + std::to_string(static_cast<int64_t>(at.x)) +
                                " y=" + std::to_string(static_cast<int64_t>(at.y));
        note(::godot::String::utf8(tap.c_str()));
        if (!hud_.press(point, hud_now_, hud_clock_, failed)) {
            fail_hud(failed);
            return;
        }
    } else {
        if (!hud_down_) {
            return;  // a release without its press: the press went to the view before the HUD was live
        }
        hud_finger_ = at;
        // THE DROP LANDS WHERE THE FINGER LETS GO (B0275, m.html 821 drop reads toUZ and fits at the release
        // point): a lifted thing aims at the release point first - the ground under it, the place, the stow
        // bridge's verdict on that place - so a release that came before the next frame never goes home.
        if (hud_.dragging()) {
            drag_ground(hud_now_.drag);
            if (!hud_.move(point, hud_now_, failed)) {
                fail_hud(failed);
                return;
            }
            bridge_stow(hud_now_.drag);
        }
        hud_down_ = false;
        if (!hud_.release(point, hud_now_, hud_clock_, requests, failed)) {
            fail_hud(failed);
            return;
        }
    }
    // a press and a release render at once in the mockup (render() after doAct), and so does every move of a lifted
    // thing or a held grip (pointermove, m.html 1006 and 1007): the next fill_hud samples again. A let go grip does not.
    if ((!moved && !window_let_go) || (moved && (hud_.dragging() || hud_.holding_window()))) {
        hud_refresh_.force();
    }
    send_hud(requests);
    node_->queue_redraw();
}

void VivariumView::tick_hud(double elapsed_s) {
    // The HUD's clock is the real time of the frames, never more than hud_frame_dt_max a frame (m.html 1025): long
    // presses fire, a held can pours on, and a stalled frame moves none of them further than that.
    const double step = hud_refresh_.step(elapsed_s);
    hud_dt_ = step;
    hud_clock_ += step;
    // the cloud banks run on the same seconds, each weighed by the wind drift along the camera's right: a turn of
    // the wind or of the camera bends the run, never makes it jump (D2293)
    hud_cloud_run_ += step * cloud_drift_share(scene_, resources_.get_sky());
    if (!hud_live()) {
        return;
    }
    // render(false) every hud_refresh_ms, and in every frame while a finger holds the can or the system ring or the
    // pruning mode is open (m.html 1037): fill_hud samples the simulation's readings when the cadence says so (T10).
    hud_refresh_.advance(step, hud_.pouring() || hud_.holding_system() || hud_.mode() == GODOT_HUD_MODE_PRUNE);
    std::string failed;
    std::vector<VivariumHudRequest> requests;
    if (!hud_.tick(hud_clock_, elapsed_s, hud_now_, requests, failed)) {
        fail_hud(failed);
        return;
    }
    // flora.sap.v1 carries no flow yet (N0457): while the bridge's tap runs on a living birch the trough fills by the
    // mockup's tick at the pace the hourglass shows, 'COCHO CHEIO' once it is full.
    bool dead = false;
    for (const std::string& sign : hud_now_.game.signs) {
        dead = dead || sign == "dead";
    }
    if (hud_bridge_sap_running_ && !dead) {
        const double factor = hud_now_.sheet.glass.factor;  // bridge_pace: flora.pace.v1, else asked, else S.factor
        // THE CROSSING IS AN EDGE: the line "hud sap full" names the litres and the factor the trough filled at,
        // so the toast 'COCHO CHEIO' is read from the log and not guessed from the time since the bore.
        if (hud_carry_.flow(step, factor, hud_bridge_sap_cap_l_, hud_bridge_sap_l_)) {
            note("hud sap full: l=" + ::godot::String::num(hud_bridge_sap_l_, GODOT_LOG_DECIMALS) +
                 " factor=" + ::godot::String::num(factor, GODOT_LOG_DECIMALS));
            if (!hud_.toast(VivariumHud::TOAST_VESSEL_FULL, {}, VivariumHud::COLOUR_SAP, failed)) {
                fail_hud(failed);
                return;
            }
        }
    }
    send_hud(requests);
}

void VivariumView::send_hud(const std::vector<VivariumHudRequest>& requests) {
    for (const VivariumHudRequest& request : requests) {
        const std::string kind = request.kind != nullptr ? request.kind : "";
        // THE CAN POURS ONCE, AT ITS RELEASE (m.html 1014-1015). A measure only grows the pour the HUD previews
        // (S.pour); the release sends the litres held times hud_bridge_hand_mm_per_l to Regar's port in one
        // irrigation, the patch in focus selected first where none is, and only a pour the port took leaves the can
        // (it.fill -= round(mm / 25)). Whether the can is empty after it picks the toast of the arrival.
        if (kind == VivariumHudRequest::ASK_POUR || kind == VivariumHudRequest::ASK_POUR_END) {
            if (selected_place_ == GODOT_NO_PLACE && !start_.patches.empty()) {
                select(patch_focus_place(selected_place_, start_.patches.front().place));
            }
        }
        if (kind == VivariumHudRequest::ASK_POUR) {
            hud_hand_.pour();
        } else if (kind == VivariumHudRequest::ASK_POUR_END) {
            const HandThing* can = hud_now_.hand.thing;
            const double fill_l = can != nullptr ? can->fill_l : 0.0;
            const double litres = hud_hand_.pour_end(fill_l);
            const bool emptied = litres > 0.0 && litres >= fill_l;
            const bool taken = water(litres * hud_hand_.mm_per_l(), emptied);
            const bool left = taken && can != nullptr && hud_carry_.poured(can->key, litres);
            note("hud pour end litres=" + ::godot::String::num(litres, 0) + (left ? " from the can" : "") +
                 (left && emptied ? ", the can empty" : ""));
        } else if (kind == VivariumHudRequest::ASK_CUT) {
            // The prune mark names the internode: the cut goes to the segment port, ase-phytomer decides what falls.
            const HostStatus status = resources_.prune(request.index);
            note("hud cut organ=" + ::godot::String::num_uint64(request.index) +
                 " result=" + GodotHostResourceManager::status_name(status));
            // usecase:100365 prune a branch (m.html 953): PODOU UM GALHO, and without another marked shoot the view
            // turns to focus (TOOLS.prune.ok()). The marks of this frame still count the shoot just cut.
            if (status == ase::kernel::HostStatusOk) {
                std::string failed;
                if (!hud_.toast(VivariumHud::TOAST_PRUNED, {}, VivariumHud::COLOUR_CUT, failed)) {
                    fail_hud(failed);
                    return;
                }
                if (hud_now_.hand.prune_left <= 1u) {
                    hud_.put_away();
                }
            }
        } else if (kind == VivariumHudRequest::ASK_VERB) {
            send_hud_verb(request);
        } else if (kind == VivariumHudRequest::ASK_DEBUG) {
            send_hud_debug(request.debug);
        } else if (kind == VivariumHudRequest::ASK_RESET) {
            restart();  // the system ring held to its end: the host anew, nothing after it in this batch
            return;
        } else if (kind == VivariumHudRequest::ASK_TAKE || kind == VivariumHudRequest::ASK_EQUIP || kind == VivariumHudRequest::ASK_STOW ||
                   kind == VivariumHudRequest::ASK_SEND) {
            // flora.carry.v1 and flora.stow.v1 file no record yet (N0451, N0452): the bridge's inventory follows
            // the act by the mockup's rules, a refusal leaves every thing where it lies.
            const bool moves = kind == VivariumHudRequest::ASK_STOW || kind == VivariumHudRequest::ASK_SEND;
            open_gap(moves ? GODOT_HUD_GAP_STOW : GODOT_HUD_GAP_CARRY, request.kind);
            std::string done;
            const bool applied = hud_carry_.apply(request, done);
            note(::godot::String(("hud carry " + kind + (applied ? " done: " : " refused: ") + done).c_str()));
        } else if (kind == VivariumHudRequest::ASK_SOW) {
            // sowTap (usecase:100388 sow a new seed): flora.carry.v1 files no seed yet (N0451). The ground under the
            // tap, the birch's sign and the pouch decide; SEMENTE NA TERRA turns the view to focus (sowNow).
            open_gap(GODOT_HUD_GAP_CARRY, request.kind);
            bool dead = false;
            for (const std::string& sign : hud_now_.game.signs) {
                dead = dead || sign == "dead";
            }
            float x = 0.0f;
            float z = 0.0f;
            float nearest_px = -1.0f;
            const bool picked = pick_ground(hud_frame_.xform(request.point), x, z, nearest_px);
            const char* toast = nullptr;
            std::string done;
            std::string failed;
            const bool sown = hud_carry_.sow(picked, ::godot::Vector3(x, 0.0f, z), dead, toast, done);
            if (sown) {
                hud_.put_away();
            }
            note(::godot::String(("hud sow " + std::string(sown ? "done: " : "refused: ") + done).c_str()));
            if (toast != nullptr &&
                !hud_.toast(toast, {}, sown ? VivariumCarryBridge::COLOUR_LIFE : GODOT_HUD_COLOUR_REFUSED, failed)) {
                fail_hud(failed);
                return;
            }
        } else if (kind == VivariumHudRequest::ASK_BROOK) {
            // brookTap (usecase:100399 carry brook water): flora.carry.v1 files no can fill and flora.stream.v1 takes
            // no water from the brook yet (N0458, N0451). The bridge fills the can of the hand by the mockup's rule
            // when the tap lies on the first brook's line; the brook's level stays the stream port's.
            open_gap(VivariumHud::GAP_FILL, request.kind);
            float x = 0.0f;
            float z = 0.0f;
            float nearest_px = -1.0f;
            const bool picked = pick_ground(hud_frame_.xform(request.point), x, z, nearest_px);
            const std::vector<VivariumStreamView>& streams = resources_.get_streams();
            const bool laid = !streams.empty();
            const ::godot::Vector3 from = laid ? ::godot::Vector3(static_cast<float>(streams.front().x), 0.0f,
                                                                  static_cast<float>(streams.front().z))
                                               : ::godot::Vector3();
            const ::godot::Vector3 to = laid ? ::godot::Vector3(static_cast<float>(streams.front().end_x), 0.0f,
                                                                static_cast<float>(streams.front().end_z))
                                             : ::godot::Vector3();
            const char* toast = nullptr;
            double litres = 0.0;
            std::string done;
            std::string failed;
            const bool filled =
                hud_carry_.brook(picked, ::godot::Vector3(x, 0.0f, z), laid, from, to, toast, litres, done);
            note(::godot::String(("hud brook " + std::string(filled ? "done: " : "refused: ") + done).c_str()));
            // The values stand named before the test (-Wdangling-pointer, as in godot_vivarium_water.cpp).
            const std::vector<HudPortValue> held{HudPortValue{"l", hud_decimals(litres, 0)}};
            if (toast != nullptr && !hud_.toast(toast, held, VivariumHud::COLOUR_WATER, failed)) {
                fail_hud(failed);
                return;
            }
        } else if (kind == VivariumHudRequest::ASK_TRUNK) {
            // flora.sap.v1 files no tap yet (N0457): the bridge's trunkTap hangs the trough, the tap runs from 0 L.
            open_gap(GODOT_HUD_GAP_SAP, request.kind);
            const char* toast = nullptr;
            std::string done;
            std::string failed;
            const bool tapped = hud_carry_.tap(toast, done);
            if (tapped) {
                hud_bridge_sap_running_ = true;
                hud_bridge_sap_l_ = 0.0;
                hud_.put_away();
            }
            note(::godot::String(("hud sap trunk " + std::string(tapped ? "done: " : "refused: ") + done).c_str()));
            if (!hud_.toast(toast, {}, tapped ? VivariumHud::COLOUR_SAP : GODOT_HUD_COLOUR_REFUSED, failed)) {
                fail_hud(failed);
                return;
            }
        } else if (kind == VivariumHudRequest::ASK_PLUG) {
            // plugTap: the peg closes the bore, the trough keeps what ran (f1(l) in the toast), the tap stops.
            open_gap(GODOT_HUD_GAP_SAP, request.kind);
            const double held_l = hud_bridge_sap_l_;
            std::string done;
            std::string failed;
            const bool plugged = hud_carry_.plug(hud_bridge_sap_running_, held_l, done);
            note(::godot::String(("hud plug trunk " + std::string(plugged ? "done: " : "refused: ") + done).c_str()));
            if (plugged) {
                hud_bridge_sap_running_ = false;
                hud_bridge_sap_l_ = 0.0;
                hud_.put_away();
                if (!hud_.toast(VivariumHud::TOAST_SAP_TAKEN, {HudPortValue{"l", hud_decimals(held_l, 1)}},
                                VivariumHud::COLOUR_SAP, failed)) {
                    fail_hud(failed);
                    return;
                }
            }
        } else {
            fail_hud("a request of no known kind: " + kind);
            return;
        }
    }
}

void VivariumView::send_hud_debug(const DebugLayerHit& part) {
    // The debug layer's parts go through the same ports the old debug buttons used (godot_vivarium_debug.cpp).
    if (part.button != GODOT_HUD_DEBUG_NONE) {
        if (part.button >= debug_.buttons.size()) {
            fail_hud("debug.buttons: the part hit");
            return;
        }
        ask_pace(debug_.buttons[part.button]);
    } else if (part.jump != GODOT_HUD_DEBUG_NONE) {
        if (part.jump >= debug_.jumps.size()) {
            fail_hud("debug.jumps: the part hit");
            return;
        }
        ask_jump(debug_.jumps[part.jump], part.jump);
    } else if (part.step != GODOT_HUD_DEBUG_NONE) {
        if (part.step >= debug_.date.steps.size()) {
            fail_hud("debug.date.steps: the part hit");
            return;
        }
        step_date(debug_.date.steps[part.step]);
    } else if (part.go) {
        ask_date();
    } else if (part.now) {
        debug_.date.held = false;  // the entry shows the clock record again
        debug_.date.typing = GODOT_DATE_PART_NONE;
        note("debug date now result=ok");
    } else if (part.field != GODOT_DATE_PART_NONE) {
        pick_date(part.field);  // the typed entry: the part the keys type into
    } else if (part.key != GODOT_HUD_DEBUG_NONE) {
        if (part.key >= debug_.date.keys.size()) {
            fail_hud("debug.date.keys: the part hit");
            return;
        }
        type_date(&debug_.date.keys[part.key]);
    } else if (part.clear) {
        type_date(nullptr);
    } else if (part.relief != GODOT_HUD_DEBUG_NONE) {
        if (part.relief >= debug_.reliefs.size()) {
            fail_hud("debug.reliefs: the part hit");
            return;
        }
        ask_shape(debug_.reliefs[part.relief]);
    }
}

void VivariumView::open_gap(const char* gap, const char* act) {
    // The edge of the bridge: logged once per id, never shown. The picture stays whole (operator decision
    // 2026-11-10); the request or port field (act) is named in the log line so the register task can find its
    // reader, and each one of an id is logged once of its own.
    const std::string key = std::string(gap) + "/" + (act != nullptr ? act : "");
    for (const std::string& each : hud_gaps_) {
        if (each == key) {
            return;
        }
    }
    hud_gaps_.push_back(key);
    note(::godot::String("hud gap=") + gap + " result=backend_open" +
         (act != nullptr ? ::godot::String(" request=") + act : ::godot::String()));
}

void VivariumView::close_gap(const char* gap, const char* act) {
    // The closing edge: the port carries the value now. Logged once, and the key leaves the list, so a port that
    // falls silent again opens the gap on a fresh edge.
    const std::string key = std::string(gap) + "/" + (act != nullptr ? act : "");
    for (size_t index = 0; index < hud_gaps_.size(); ++index) {
        if (hud_gaps_[index] == key) {
            hud_gaps_.erase(hud_gaps_.begin() + static_cast<std::ptrdiff_t>(index));
            note(::godot::String("hud gap=") + gap + " result=ok" +
                 (act != nullptr ? ::godot::String(" request=") + act : ::godot::String()));
            return;
        }
    }
}

}  // namespace ase::adp::godot
