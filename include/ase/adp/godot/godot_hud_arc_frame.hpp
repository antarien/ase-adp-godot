#pragma once

/**
 * ASE GODOT ADAPTER - HUD SKY ARC FRAMES
 *
 * @file        godot_hud_arc_frame.hpp
 * @brief       What the sky arc shows this frame: SkyArcFrame, ArcRingFrame and ArcSignFrame, ready from the bridge
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, "Bausteine im Adapter". Frames without logic: the
 *              bridge (VivariumView::bridge_arc) fills them from flora.arc.v1, flora.almanac.v1 and flora.event_code.v1,
 *              or, while a port carries no record, from the mockup's values in SHA (hud_bridge_arc_*); the units
 *              draw them and never ask whether a port delivered. Every share is the owner's: -1 is the left edge of a
 *              ring's window, 0 its anchor, +1 its right edge, and `now` is the share of now against the same anchor.
 *              A kind is its name (GODOT_ARC_KIND_*), found by the bridge against the codes of the calendar.
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

#include <ase/adp/godot/arc/types.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

/** One occurrence on a ring as the owner placed it. */
struct ArcSignFrame {
    std::string kind;     // GODOT_ARC_KIND_* by the codes of the calendar
    double body = 0.0;    // which one of its kind: the moon, the conjunction, the season, the period, the phase
    double share = 0.0;   // its place as a share of the window against the anchor
    double span = 0.0;    // its length as a share of the window, 0 for a point
};

/** One ring: the share of now and the occurrences laid on it. */
struct ArcRingFrame {
    double now = 0.0;                  // the share of now against the anchor of the set, every frame
    std::vector<ArcSignFrame> signs;   // the occurrences of the set on this ring
};

/** What the sky arc shows this frame. */
struct SkyArcFrame {
    ArcRingFrame rings[GODOT_ARC_RINGS];  // day, month, year in the order GODOT_ARC_SLOT_*
    uint64_t seq = 0;                     // the number of the set the rings stand on: a change lays them anew
    std::string next_kind;                // the kind of the next event by name; empty while none
    double next_body = 0.0;               // its body
    double next_in_h = 0.0;               // game hours until it, as the owner computed them
    double next_in_d = 0.0;               // whole game days until it, as the owner computed them; from 1 on the line names them
    double period = 0.0;                  // the SURA date: the code of the day period
    double year = 0.0;                    // the SURA date: VECTUM
    double epoch = 0.0;                   // the SURA date: AETRIX
    double cycle = 0.0;                   // the SURA date: CYCLUM
    double month = 0.0;                   // the SURA date: LUNBREX
    double month_day = 0.0;               // the SURA date: TUMBRAE
    double hour = 0.0;                    // the SURA date: HORAX, the whole hour is shown
    double elapsed = 0.0;                 // seconds of the view's clock: the frame time of the flank line
    bool still = false;                   // every motion held
};

}  // namespace ase::adp::godot
