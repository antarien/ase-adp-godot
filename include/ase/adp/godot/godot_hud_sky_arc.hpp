#pragma once

/**
 * ASE GODOT ADAPTER - HUD SKY ARC
 *
 * @file        godot_hud_sky_arc.hpp
 * @brief       SkyArc: the sky arc of the top band, three rigid rings turning under a fixed needle with the readout
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, layer 3a SkyArc in front of SkyBand (hud.modes
 *              names the layer `arc` wherever it names `band`). The group of the mockup's arcStatic and arcFrame: the
 *              three rings ArcRing (year, month, day: the highest drawn first), the needle ArcNeedle and the two lines
 *              ArcReadout. Everything it shows stands in SkyArcFrame, which the bridge (VivariumView::bridge_arc) fills
 *              from flora.arc.v1, flora.almanac.v1 and flora.event_code.v1 or, while those carry no record, from the
 *              mockup's values in SHA (hud_bridge_arc_*); the arc is always drawn whole and never asks whether a port
 *              delivered.
 *
 *              THE ADAPTER ONLY TURNS. A ring turns by the share of now times its half span, a sign stands at its share
 *              minus the share of now times that span (arc_disc_angle). No hour, no day and no clock stands in any unit
 *              of the arc (ADAPTER_COMPUTES_GAME_TIME, ADAPTER_DECIDES_FROM_CLOCK).
 *
 *              THE TOUCH. hit() names GODOT_HUD_ACT_SKY inside the rect hud_arc_hit_*, which opens the calendar sheet as
 *              the band does; the sheet's mode keeps the arc standing and turning.
 *
 *              THE FLANK. draw writes `debug arc now day= lun= year= seq= t=` every frame; the signs and the readout
 *              add their own lines. The view takes them with take_flank after the frame and logs them.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_067
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    carrier N0448
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_arc_frame.hpp>
#include <ase/adp/godot/godot_hud_arc_needle.hpp>
#include <ase/adp/godot/godot_hud_arc_readout.hpp>
#include <ase/adp/godot/godot_hud_arc_ring.hpp>
#include <ase/adp/godot/godot_hud_arc_sign.hpp>
#include <ase/adp/godot/godot_hud_ui_arc_fade.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>

#include <string>
#include <vector>

namespace ase::adp::godot {

/** The sky arc. */
class SkyArc {
public:
    /** Binds the fade, the signs, the rings, the needle and the readout; false with the missing name in `failed`. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** Draws the arc of `now` under `frame`, the transform of the top band; false with the reason in `failed`. */
    bool draw(UiPen& canvas, const ::godot::Transform2D& frame, const SkyArcFrame& now, std::string& failed);

    /** GODOT_HUD_ACT_SKY inside the arc's rect (the mockup's units of the top band), else nullptr. */
    const char* hit(const ::godot::Vector2& point) const;

    /** The rect a tap on the arc opens the calendar sheet through. */
    ::godot::Rect2 arc_place() const;

    /** The rect of the needle. */
    ::godot::Rect2 needle_place() const;

    /** The flank lines of the frames drawn since the last call, in their order; the list is emptied. */
    std::vector<std::string> take_flank();

private:
    UiArcFade fade_;
    ArcSign sign_;
    ArcRing rings_[GODOT_ARC_RINGS];
    ArcNeedle needle_;
    ArcReadout readout_;
    double hit_x_ = 0.0;   // hud_arc_hit_x
    double hit_y_ = 0.0;   // hud_arc_hit_y
    double hit_w_ = 0.0;   // hud_arc_hit_w
    double hit_h_ = 0.0;   // hud_arc_hit_h
    std::vector<std::string> flank_;  // the lines written since the last take_flank
};

}  // namespace ase::adp::godot
