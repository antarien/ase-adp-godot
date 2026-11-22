/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW FLATTENING
 *
 * @file        godot_vivarium_flatten.cpp
 * @brief       VivariumView - Aplanar: the flatten mode and the disc the player levels where he taps
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.8, operator order of 2026-10-25 (flatten an area of
 *              the ground): the player levels the ground at a place of his choosing with the mouse or
 *              the finger. In the flatten mode a tap on the ground sends a disc of radius view.flatten_m
 *              around the terrain sample under the tap to the flatten port; ase-pl-flora lays its body,
 *              ase-terrain levels it to the height of its centre, and the view shows the new ground once
 *              the terrain port carries it - never an optimistic shape. The ground shader paints the disc as
 *              bare soil like a levelled patch (fill_ground). Nivelar stays the level of the selected
 *              patch (godot_vivarium_level.cpp); the tap that leads here is godot_vivarium_input.cpp.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_015
 * @category    ecs/module
 * @usecase     usecase:5939 - adapt area terrain
 * @actor       Game Areal Plate (AREAL)
 * @created     2026-10-25
 * @modified    2026-11-13
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/host/types.hpp>
#include <ase/kernel/kernel_types.hpp>

#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/viewport.hpp>

#include <ase/utils/clock.hpp>

#include <cstdint>
#include <ctime>
#include <string>

namespace ase::adp::godot {

using ase::kernel::HostStatus;

uint64_t thread_cpu_usec() {
    // the calling thread's own CPU clock in the base unit and divisor of ase-utils' monotonic_nanos()
    timespec spent{};
    if (::clock_gettime(CLOCK_THREAD_CPUTIME_ID, &spent) != 0) {
        return 0;
    }
    const int64_t nanos = static_cast<int64_t>(spent.tv_sec) * ase::utils::NANOS_PER_SECOND +
                          static_cast<int64_t>(spent.tv_nsec);
    return static_cast<uint64_t>(nanos / ase::utils::NANOS_PER_MICRO);
}

bool VivariumView::can_flatten() const {
    // A running host and a ground to tap on.
    return resources_.running() && failure_text_.is_empty() && scene_ != nullptr &&
           !resources_.get_terrain().empty();
}

void VivariumView::toggle_flattening() {
    if (!flattening_ && !can_flatten()) {
        note("flatten mode result=disabled");
        return;
    }
    flattening_ = !flattening_;
    // one mode at a time: a tap in the scene either cuts, lays a bed or levels a disc
    pruning_ = false;
    bedding_ = false;
    bed_started_ = false;
    note(::godot::String("flatten mode ") + (flattening_ ? "on" : "off"));
    node_->queue_redraw();
}

bool VivariumView::flatten(const ::godot::Vector2& at, float radius_m) {
    float x = 0.0f;
    float z = 0.0f;
    float nearest_px = -1.0f;
    if (!pick_ground(at, x, z, nearest_px)) {
        note("flatten at " + ::godot::String::num(at.x, 0) + "," + ::godot::String::num(at.y, 0) +
             " result=none nearest_px=" + ::godot::String::num(nearest_px, 1));
        return false;
    }
    ::godot::Time* clock = ::godot::Time::get_singleton();
    const uint64_t act_start_usec = clock->get_ticks_usec();
    const HostStatus status = resources_.flatten(x, z, radius_m);
    watch_ground_act("flatten", clock->get_ticks_usec() - act_start_usec);
    if (status == ase::kernel::HostStatusOk) {
        // the height is the ground's at the centre once ase-terrain levelled it (fill_ground)
        player_flats_.push_back(PlantGroundDisc{x, z, radius_m, 0.0f});
    }
    note("flatten x=" + ::godot::String::num(x, 2) + " z=" + ::godot::String::num(z, 2) +
         " radius=" + ::godot::String::num(radius_m, 2) + " result=" + GodotHostResourceManager::status_name(status));
    node_->queue_redraw();
    return status == ase::kernel::HostStatusOk;  // the HUD toasts only an act the port took (applyTerr's changed)
}

bool VivariumView::lower(float x, float z, float radius_m, float depth_m) {
    // The caller picked the ground point once (send_hud_verb): the tap's, or the planting hole's when the spade opens
    // it, so the dig and the fill of one hole land on the same ground (B0297).
    // The view shows the dug ground once the terrain port carries it; water stands where it lies at or below NN0.
    ::godot::Time* clock = ::godot::Time::get_singleton();
    const uint64_t act_start_usec = clock->get_ticks_usec();
    const HostStatus status = resources_.lower(x, z, radius_m, depth_m);
    watch_ground_act("lower", clock->get_ticks_usec() - act_start_usec);
    note("lower x=" + ::godot::String::num(x, 2) + " z=" + ::godot::String::num(z, 2) +
         " radius=" + ::godot::String::num(radius_m, 2) + " depth=" + ::godot::String::num(depth_m, 2) +
         " result=" + GodotHostResourceManager::status_name(status));
    node_->queue_redraw();
    return status == ase::kernel::HostStatusOk;
}

bool VivariumView::raise(float x, float z, float radius_m, float height_m) {
    // the ground point as lower() takes it: the tap's, or the planting hole's when amontoar fills it (B0297)
    ::godot::Time* clock = ::godot::Time::get_singleton();
    const uint64_t act_start_usec = clock->get_ticks_usec();
    const HostStatus status = resources_.raise(x, z, radius_m, height_m);
    watch_ground_act("raise", clock->get_ticks_usec() - act_start_usec);
    note("raise x=" + ::godot::String::num(x, 2) + " z=" + ::godot::String::num(z, 2) +
         " radius=" + ::godot::String::num(radius_m, 2) + " height=" + ::godot::String::num(height_m, 2) +
         " result=" + GodotHostResourceManager::status_name(status));
    node_->queue_redraw();
    return status == ase::kernel::HostStatusOk;
}

bool VivariumView::loosen(float x, float z, float half_m, float depth_m) {
    // The hoe stroke goes to the owner of the ground (B0649): ase-pl-flora files it as a cultivation order, ase-terrain
    // loosens the topsoil and moves no height. NotFound names a plugin set without flora.loosen.v1 in the log line.
    ::godot::Time* clock = ::godot::Time::get_singleton();
    const uint64_t act_start_usec = clock->get_ticks_usec();
    const HostStatus status = resources_.loosen(x, z, half_m, depth_m);
    watch_ground_act("loosen", clock->get_ticks_usec() - act_start_usec);
    note("loosen x=" + ::godot::String::num(x, 2) + " z=" + ::godot::String::num(z, 2) +
         " half=" + ::godot::String::num(half_m, 2) + " depth=" + ::godot::String::num(depth_m, 2) +
         " result=" + GodotHostResourceManager::status_name(status));
    node_->queue_redraw();
    return status == ase::kernel::HostStatusOk;
}

void VivariumView::watch_ground_act(const char* op, uint64_t act_usec) {
    // A new act while the last one is still watched ends that watch with the frames it has; its line stands first.
    if (!ground_watch_.op.empty()) {
        report_ground_watch();
    }
    ground_watch_.op = op;
    ground_watch_.frames_left = GODOT_GROUND_WATCH_FRAMES;
    ground_watch_.frames = 0;
    ground_watch_.act_usec = act_usec;
    ground_watch_.max_usec = 0;
    ground_watch_.host_max_usec = 0;
    ground_watch_.follow_max_usec = 0;
    ground_watch_.scene_max_usec = 0;
    ground_watch_.hud_max_usec = 0;
    ground_watch_.process_max_usec = 0;
    ground_watch_.rest_max_usec = 0;
    ground_watch_.draw_max_usec = 0;
    ground_watch_.draw_cpu_max_usec = 0;
    ground_watch_.cpu_at_max_usec = 0;
    // the frames before the act: the span running now and the one before it cover at least the last
    // GODOT_GROUND_WATCH_FRAMES intervals
    ground_watch_.before_usec = ground_watch_.span_max_usec > ground_watch_.span_prev_usec
                                    ? ground_watch_.span_max_usec
                                    : ground_watch_.span_prev_usec;
    ground_watch_.span_max_usec = 0;
    ground_watch_.span_prev_usec = 0;
    ground_watch_.span_frames = 0;
    // the compute time per unit and schedule from here on: the cursors are set now, the line reads them at the end
    ::godot::Time* clock = ::godot::Time::get_singleton();
    const uint64_t units_start_usec = clock->get_ticks_usec();
    lay_cycles(ground_watch_.units);
    ground_watch_.units_ready = cycle_measure_valid(ground_watch_.units) && resources_.measure_begin(ground_watch_.units);
    ground_watch_.units_begin_usec = clock->get_ticks_usec() - units_start_usec;
    // the viewport's render time is measured only once asked for; the frame lines read it (watch_ground_frame)
    ::godot::RenderingServer::get_singleton()->viewport_set_measure_render_time(
        node_->get_viewport()->get_viewport_rid(), true);
}

void VivariumView::watch_ground_frame(uint64_t frame_start_usec) {
    // The interval from the previous frame's start to this one's holds everything between them: the input that
    // carried the act, the port call, the host's tick that took the delta and the drawing of the new ground.
    // The main thread's CPU time over the same interval: what it computed of it; the rest it waited for a core.
    const uint64_t last_usec = ground_watch_.last_usec;
    const uint64_t last_cpu_usec = ground_watch_.last_cpu_usec;
    const uint64_t frame_cpu_usec = thread_cpu_usec();
    ground_watch_.last_usec = frame_start_usec;
    ground_watch_.last_cpu_usec = frame_cpu_usec;
    if (last_usec == 0 || frame_start_usec < last_usec) {
        return;
    }
    const uint64_t interval_usec = frame_start_usec - last_usec;
    const uint64_t cpu_usec = last_cpu_usec != 0 && frame_cpu_usec >= last_cpu_usec ? frame_cpu_usec - last_cpu_usec
                                                                                    : 0;
    // What follows the previous _process up to this start: the input that carried an act, Godot drawing the canvas
    // and rendering the scene. Known only when the previous frame measured its _process.
    const uint64_t process_usec = ground_watch_.last_process_usec;
    ground_watch_.last_process_usec = 0;
    // The HUD's canvas drawing since the previous start is one share of that rest; the other is Godot's own steps.
    const uint64_t draw_usec = ground_watch_.last_draw_usec;
    const uint64_t draw_cpu_usec = ground_watch_.last_draw_cpu_usec;
    ground_watch_.last_draw_usec = 0;
    ground_watch_.last_draw_cpu_usec = 0;
    if (!ground_watch_.op.empty() && draw_usec > ground_watch_.draw_max_usec) {
        ground_watch_.draw_max_usec = draw_usec;
    }
    if (!ground_watch_.op.empty() && draw_cpu_usec > ground_watch_.draw_cpu_max_usec) {
        ground_watch_.draw_cpu_max_usec = draw_cpu_usec;
    }
    if (!ground_watch_.op.empty() && process_usec != 0 && process_usec <= interval_usec &&
        interval_usec - process_usec > ground_watch_.rest_max_usec) {
        ground_watch_.rest_max_usec = interval_usec - process_usec;
    }
    if (!ground_watch_.op.empty() && interval_usec > GODOT_GROUND_WATCH_BUDGET_USEC) {
        // An interval over the budget names its shares from Godot's own clocks: its whole process step with the
        // canvas drawing, the physics step, the render setup and the viewport's CPU and GPU render time.
        const ::godot::Performance* perf = ::godot::Performance::get_singleton();
        ::godot::RenderingServer* render = ::godot::RenderingServer::get_singleton();
        const ::godot::RID viewport = node_->get_viewport()->get_viewport_rid();
        note(::godot::String(("ground_frame op=" + ground_watch_.op + " frame=" + std::to_string(ground_watch_.frames) +
                              " interval_usec=" + std::to_string(interval_usec) +
                              " cpu_usec=" + std::to_string(cpu_usec) +
                              " view_process_usec=" + std::to_string(process_usec) +
                              " hud_draw_usec=" + std::to_string(draw_usec) +
                              " hud_draw_cpu_usec=" + std::to_string(draw_cpu_usec))
                                 .c_str()) +
             " godot_process_s=" + ::godot::String::num(perf->get_monitor(::godot::Performance::TIME_PROCESS), 4) +
             " godot_physics_s=" +
             ::godot::String::num(perf->get_monitor(::godot::Performance::TIME_PHYSICS_PROCESS), 4) +
             " render_setup_ms=" + ::godot::String::num(render->get_frame_setup_time_cpu(), 2) +
             " render_cpu_ms=" + ::godot::String::num(render->viewport_get_measured_render_time_cpu(viewport), 2) +
             " render_gpu_ms=" + ::godot::String::num(render->viewport_get_measured_render_time_gpu(viewport), 2));
    }
    if (ground_watch_.op.empty()) {
        // between acts: the spans of GODOT_GROUND_WATCH_FRAMES intervals the next act holds its frames against
        if (interval_usec > ground_watch_.span_max_usec) {
            ground_watch_.span_max_usec = interval_usec;
        }
        ++ground_watch_.span_frames;
        if (ground_watch_.span_frames >= GODOT_GROUND_WATCH_FRAMES) {
            ground_watch_.span_prev_usec = ground_watch_.span_max_usec;
            ground_watch_.span_max_usec = 0;
            ground_watch_.span_frames = 0;
        }
        return;
    }
    if (interval_usec > ground_watch_.max_usec) {
        ground_watch_.max_usec = interval_usec;
        ground_watch_.cpu_at_max_usec = cpu_usec;
    }
    ++ground_watch_.frames;
    --ground_watch_.frames_left;
    if (ground_watch_.frames_left == 0) {
        report_ground_watch();
    }
}

void VivariumView::watch_ground_host(uint64_t host_usec) {
    // The host advance of a frame lies inside the interval the next frame measures; while a watch runs it is the
    // share of the host's tick in the frames after the act.
    if (!ground_watch_.op.empty() && host_usec > ground_watch_.host_max_usec) {
        ground_watch_.host_max_usec = host_usec;
    }
}

void VivariumView::watch_ground_phases(uint64_t follow_usec, uint64_t scene_usec, uint64_t hud_usec,
                                       uint64_t process_usec) {
    // The view's own steps of a frame; the whole _process is kept for the next frame, whose interval holds it, so
    // the share after it - drawing and rendering - is that interval minus this _process.
    ground_watch_.last_process_usec = process_usec;
    if (ground_watch_.op.empty()) {
        return;
    }
    if (follow_usec > ground_watch_.follow_max_usec) {
        ground_watch_.follow_max_usec = follow_usec;
    }
    if (scene_usec > ground_watch_.scene_max_usec) {
        ground_watch_.scene_max_usec = scene_usec;
    }
    if (hud_usec > ground_watch_.hud_max_usec) {
        ground_watch_.hud_max_usec = hud_usec;
    }
    if (process_usec > ground_watch_.process_max_usec) {
        ground_watch_.process_max_usec = process_usec;
    }
}

void VivariumView::report_ground_watch() {
    const bool held = ground_watch_.max_usec <= GODOT_GROUND_WATCH_BUDGET_USEC;
    note(::godot::String(("ground_act op=" + ground_watch_.op + " act_usec=" + std::to_string(ground_watch_.act_usec) +
                          " frames=" + std::to_string(ground_watch_.frames) +
                          " max_frame_usec=" + std::to_string(ground_watch_.max_usec) +
                          " max_frame_cpu_usec=" + std::to_string(ground_watch_.cpu_at_max_usec) +
                          " max_host_usec=" + std::to_string(ground_watch_.host_max_usec) +
                          " max_follow_usec=" + std::to_string(ground_watch_.follow_max_usec) +
                          " max_scene_usec=" + std::to_string(ground_watch_.scene_max_usec) +
                          " max_hud_usec=" + std::to_string(ground_watch_.hud_max_usec) +
                          " max_process_usec=" + std::to_string(ground_watch_.process_max_usec) +
                          " max_rest_usec=" + std::to_string(ground_watch_.rest_max_usec) +
                          " max_draw_usec=" + std::to_string(ground_watch_.draw_max_usec) +
                          " max_draw_cpu_usec=" + std::to_string(ground_watch_.draw_cpu_max_usec) +
                          " before_max_frame_usec=" + std::to_string(ground_watch_.before_usec) +
                          " budget_usec=" + std::to_string(GODOT_GROUND_WATCH_BUDGET_USEC) +
                          " result=" + (held ? "held" : "exceeded")).c_str()));
    if (ground_watch_.units_ready) {
        // which unit spent the host's share of the watched frames: the cycle line over the span since the act
        std::string units_line;
        resources_.measure_cycle(ground_watch_.units, units_line);
        note(::godot::String(("ground_act_units op=" + ground_watch_.op +
                              " begin_usec=" + std::to_string(ground_watch_.units_begin_usec) + " " + units_line)
                                 .c_str()));
        ground_watch_.units_ready = false;
    }
    ground_watch_.op.clear();
    ground_watch_.frames_left = 0;
}

}  // namespace ase::adp::godot
