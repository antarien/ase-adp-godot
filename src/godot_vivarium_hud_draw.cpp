/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD DRAWING
 *
 * @file        godot_vivarium_hud_draw.cpp
 * @brief       VivariumView::draw_hud - the HUD over the scene each frame, the debug layer's frame and the error panel
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.5: draw_hud lays the HUD's frame on the placed layout (place_hud),
 *              lets fill_hud (godot_vivarium_hud.cpp) fill every block's frame from the ports, lets the HUD resolve
 *              its mode and draws it with the pen at the screen's pixels. After each drawn frame it logs the edge of
 *              the carry bridge for the containers, lays the cloud banks into the sky of the 3D scene, logs the
 *              places a tap reaches the HUD through (note_hud_places, godot_vivarium_hud_place.cpp) and, behind
 *              them, the edge of a mode the HUD opened. In the frames a ground act watches it writes the CPU shares
 *              of each step. fill_hud_debug fills the debug layer's frame: the identity of the build, the clock, the
 *              patch in focus and the date entry. A HUD that cannot draw names why on the error panel (fail_hud).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-20
 * @modified    2026-11-20
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_config_read.hpp>
#include <ase/adp/godot/godot_hud_ui_pen_class.hpp>
#include <ase/adp/godot/godot_patch_focus.hpp>
#include <ase/adp/godot/godot_plant_clouds.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/adp/godot/patch/types.hpp>

#include <godot_cpp/variant/utility_functions.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

void VivariumView::fill_hud_debug(const VivariumPatchView* patch) {
    // The debug layer: the identity of the build, the clock, the chosen patch and the date the entry shows.
    DebugLayerFrame& debug = hud_now_.debug;
    // The clock record of the last sample, as the band reads it: every group the ports leave empty is the bridge's
    // (N0460).
    VivariumClockView clock = hud_shown_.clock;
    bridge_clock(clock);
    debug.stage = patch != nullptr ? patch->stage : GODOT_STAGE_NONE;
    // The TEMPO chip of the factor the clock runs at, the one the hourglass fills: bridge_pace wrote it above
    // (N0459). The RELEVO chip of the relief that stands: the boot's or the one granted last (ask_shape).
    debug.factor = hud_now_.sheet.glass.factor;
    debug.height_scale_m = debug_.height_scale_m;
    // The adapter's version as the stage wrote it into build.identity (stage_native.py identity_text): a version
    // passed as a compile define would stand on the command line of every unit of the adapter.
    ::godot::String adapter;
    if (read_text(identity_, "adapter_version", adapter)) {
        debug.adapter = adapter.utf8().get_data();
    }
    ::godot::String client;
    if (read_text(identity_, "client_commit", client)) {
        debug.client = client.utf8().get_data();
    }
    debug.units = static_cast<uint64_t>(resources_.get_bundle().units.size());
    debug.day = clock.year_day;
    debug.hour = clock.hour;
    debug.simulation_s = hud_shown_.simulation_s;
    debug.steps = hud_shown_.steps;
    debug.patch = patch != nullptr;
    if (patch != nullptr) {
        debug.cover = patch->coverage;
        debug.soil_mm = patch->soil_mm;
        debug.soil_rel = patch->soil_rel;
    }
    // The salt of every chunk as the salt port laid it (H.9, I-4): summed here for the status line only, every
    // value is ase-hydro's, ase-pedosphere's and ase-gis's; read every frame, so the line follows the port.
    const std::vector<VivariumSaltView>& salt = resources_.get_salt();
    debug.salt_chunks = static_cast<uint64_t>(salt.size());
    debug.salt_hyd_kg = 0.0;
    debug.salt_soil_kg = 0.0;
    debug.salt_kg = 0.0;
    for (const VivariumSaltView& chunk : salt) {
        debug.salt_hyd_kg += chunk.hyd_kg;
        debug.salt_soil_kg += chunk.soil_kg;
        debug.salt_kg += chunk.sum_kg;
    }
    static_cast<void>(shown_date(debug.date));
    debug.typing = debug_.date.typing;
    debug.elapsed = hud_clock_;
}

void VivariumView::draw_hud() {
    std::string failed;
    // THE SHARES OF THE DRAWING (P0692): the frame line of the ground watch names the canvas time as one number
    // (hud_draw_cpu_usec, about 35 ms of CPU in every frame in run 20261113-033131-p0690-cpu). The main thread's CPU
    // time at each step boundary says which step carries it; the watched frames after a ground act write the shares.
    const uint64_t cpu_begin = thread_cpu_usec();
    place_hud();
    const uint64_t cpu_placed = thread_cpu_usec();
    // The HUD lays its shapes in mockup units under hud_frame_; the pen renders each at the screen's pixels.
    UiPen pen(*node_);
    if (!fill_hud(failed)) {
        fail_hud(failed);
        return;
    }
    const uint64_t cpu_filled = thread_cpu_usec();
    if (!hud_.resolve(hud_now_, failed)) {
        fail_hud(failed);
        return;
    }
    const uint64_t cpu_resolved = thread_cpu_usec();
    if (!hud_.draw(pen, hud_frame_, hud_now_, failed)) {
        fail_hud(failed);
        return;
    }
    draw_figure_hud(pen);  // the phone's stick and cam.mode over the HUD (07.9)
    const uint64_t cpu_drawn = thread_cpu_usec();
    // THE EDGE OF THE CARRY BRIDGE FOR THE CONTAINERS (N0451): BagWindow::take took this frame's container records
    // from VivariumCarryBridge in every mode. A container left without one (ContainerDef::backend_open) is a fault of
    // the bridge and keeps its last record in the picture; the bridge names its register id (ContainerDef::gap) once
    // on its edge with the fields of the record flora.carry.v1 will carry. The windows themselves never ask.
    const BagWindow& carried = hud_.window();
    for (const ContainerDef* container : {&carried.pack_def(), &carried.pouch_def()}) {
        if (container->backend_open()) {
            open_gap(container->gap(), "flora.carry.v1.containers(opening_cm,room_l,carry_kg,asset)");
        }
    }
    // The cloud banks of the mockup's drawSky stand behind the hills and the tree, so CloudBank lays them in
    // frame units and the sky of the 3D scene shows them (godot_plant_clouds.cpp, N04); the kind is the veil's.
    std::vector<CloudBankEllipse> banks;
    hud_.clouds().lay(CloudBankFrame{hud_now_.veil.kind, hud_cloud_run_}, banks);
    light_plant_clouds(scene_, banks, hud_.clouds().veil(), hud_frame_);
    const uint64_t cpu_clouded = thread_cpu_usec();
    note_hud_places();
    const uint64_t cpu_noted = thread_cpu_usec();
    if (!ground_watch_.op.empty()) {
        // only in the GODOT_GROUND_WATCH_FRAMES frames a ground act watches, beside its ground_frame lines; the draw
        // share is followed by the share of each step of VivariumHud::draw (HudDrawShare), so the line names the step
        // that carries the drawing's cost and not only its sum.
        std::string split = "hud_draw_split op=" + ground_watch_.op + " frame=" +
                            std::to_string(ground_watch_.frames) +
                            " place_cpu_usec=" + std::to_string(cpu_placed - cpu_begin) +
                            " fill_cpu_usec=" + std::to_string(cpu_filled - cpu_placed) +
                            " resolve_cpu_usec=" + std::to_string(cpu_resolved - cpu_filled) +
                            " draw_cpu_usec=" + std::to_string(cpu_drawn - cpu_resolved) +
                            " clouds_cpu_usec=" + std::to_string(cpu_clouded - cpu_drawn) +
                            " places_cpu_usec=" + std::to_string(cpu_noted - cpu_clouded);
        for (const HudDrawShare& share : hud_.shares()) {
            split += " draw_" + share.step + "_cpu_usec=" + std::to_string(share.cpu_usec);
        }
        note(::godot::String(split.c_str()));
    }
    // THE EDGE OF A MODE COMES AFTER ITS PLACES. One line names the mode the HUD opened and measures the patch in
    // focus, the place a tap on the birch (ACT_TREE) or a care step acts on, with the same fields the plain view's
    // select line carries (measure: place, object, game_s, soil); the window acceptance reads it where the plain view
    // writes its select line. It stands behind the place set of the same frame: the acceptance looks a place up the
    // moment it reads the edge, and in run 20261112-places-210911 the edge "hud mode=rest" stood 37 ms before the
    // rest frame's places, so the lookup found the sky mode's set and missed the system glyph (H06).
    if (hud_.mode() != hud_mode_) {
        hud_mode_ = hud_.mode();
        const uint32_t focus = start_.patches.empty() ? GODOT_NO_PLACE
                                                      : patch_focus_place(selected_place_, start_.patches.front().place);
        note("hud mode=" + ::godot::String::utf8(hud_mode_.c_str()) + " " + measure(focus));
    }
}

void VivariumView::fail_hud(const std::string& failed) {
    failure_text_ = texts_.error + ": hud " + ::godot::String::utf8(failed.c_str());
    ::godot::UtilityFunctions::push_error(failure_text_);
    note("hud result=failed reason=" + ::godot::String::utf8(failed.c_str()));
    node_->queue_redraw();
}

}  // namespace ase::adp::godot
