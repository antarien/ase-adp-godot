/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW FRAME RATE
 *
 * @file        godot_vivarium_frame_rate.cpp
 * @brief       VivariumView - the frames per second of every frame, their place on the surface and the line of an edge
 * @description Operator order 2026-11-24: the player sees the frames per second while the game runs, and a run names a
 *              rate under the floor. process() hands every frame's real distance here, before the host advances, so the
 *              number counts the frames the player saw, the start screen's among them. The place is the top band of the
 *              placed layout, as the HUD's band zone: below a camera cut-out, on every surface.
 *
 *              ONE LINE PER EDGE: `frame_rate fps=<n> floor=<n> under=<0|1>` with the first reading and with every
 *              change of the verdict, never per frame. The acceptance run reads the spans under the floor from them;
 *              which module took the time is the measure's answer (godot_vivarium_measure.cpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-24
 * @modified    2026-11-25
 * @version     00.00.02.00002 [refine]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>

#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ase::adp::godot {

void VivariumView::follow_frame_rate(double elapsed_s) {
    ::godot::Rect2 head;
    float scale = 0.0f;
    ::godot::Vector2 origin;
    if (placement_.scale > 0.0f && hud_state_.hud_.theme().zone("band", head)) {
        scale = static_cast<float>(static_cast<double>(placement_.scale) * hud_state_.hud_unit_);
        origin = ::godot::Vector2(placement_.offset_x, band_offset(layout_.bands, placement_, head.position.y));
    }
    ::godot::RenderingServer* render = ::godot::RenderingServer::get_singleton();
    const ::godot::RID viewport = node_->get_viewport()->get_viewport_rid();
    // THE VIEWPORT MEASURES ITS RENDER TIME ONLY ONCE ASKED, AND ONLY FOR THE FRAMES AFTER THE ASKING. Asked in the
    // call that prints the first edge, it was read in that same call and stood at 0.0 in every line of 2026-11-25
    // (fps 3, 7, 9, 10). Asked over the frames of the first window instead, the first edge reads a rendered frame.
    if (!hud_state_.frame_rate_.rate().known()) {
        render->viewport_set_measure_render_time(viewport, true);
    }
    if (!hud_state_.frame_rate_.frame(*node_, elapsed_s, origin, scale)) {
        return;
    }
    const HudFrameRate& rate = hud_state_.frame_rate_.rate();
    // THE EDGE NAMES WHERE THE FRAME WENT (operator order 2026-11-25): Godot's own clocks for the process step (the
    // host and the view, HUD drawing included), the physics step and the viewport's render on CPU and GPU, so a run
    // under the floor says which share to cut without a measuring mode of its own.
    const ::godot::Performance* perf = ::godot::Performance::get_singleton();
    note("frame_rate fps=" + ::godot::String::num_int64(rate.shown()) +
         " floor=" + ::godot::String::num_int64(rate.floor_rate()) + " under=" + (rate.under() ? "1" : "0") +
         " process_s=" + ::godot::String::num(perf->get_monitor(::godot::Performance::TIME_PROCESS), 4) +
         " physics_s=" + ::godot::String::num(perf->get_monitor(::godot::Performance::TIME_PHYSICS_PROCESS), 4) +
         " render_cpu_ms=" + ::godot::String::num(render->viewport_get_measured_render_time_cpu(viewport), 2) +
         " render_gpu_ms=" + ::godot::String::num(render->viewport_get_measured_render_time_gpu(viewport), 2));
}

}  // namespace ase::adp::godot
