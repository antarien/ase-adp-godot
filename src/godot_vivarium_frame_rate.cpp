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
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>

#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/vector2.hpp>

namespace ase::adp::godot {

void VivariumView::follow_frame_rate(double elapsed_s) {
    ::godot::Rect2 head;
    float scale = 0.0f;
    ::godot::Vector2 origin;
    if (placement_.scale > 0.0f && hud_.theme().zone("band", head)) {
        scale = static_cast<float>(static_cast<double>(placement_.scale) * hud_unit_);
        origin = ::godot::Vector2(placement_.offset_x, band_offset(layout_.bands, placement_, head.position.y));
    }
    if (!frame_rate_.frame(*node_, elapsed_s, origin, scale)) {
        return;
    }
    const HudFrameRate& rate = frame_rate_.rate();
    note("frame_rate fps=" + ::godot::String::num_int64(rate.shown()) +
         " floor=" + ::godot::String::num_int64(rate.floor_rate()) + " under=" + (rate.under() ? "1" : "0"));
}

}  // namespace ase::adp::godot
