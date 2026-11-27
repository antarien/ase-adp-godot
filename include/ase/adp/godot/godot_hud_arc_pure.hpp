#pragma once

/**
 * ASE GODOT ADAPTER - HUD ARC, THE PART WITHOUT A GODOT TYPE
 *
 * @file        godot_hud_arc_pure.hpp
 * @brief       The turn of a ring, the fade of its ends and the names of the codes, callable by the host test
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6. The adapter only turns: the angle of a disc is
 *              (share of the occurrence minus share of now) times the half span of its ring, the half span a measure of
 *              SHA (hud_arc_<ring>_half_deg). arc_disc_angle is that one product and nothing else; no hour, no day and
 *              no clock stands in it. arc_fade_opacity is UiArcFade's piecewise line over the stops hud_arc_fade_*
 *              (Godot knows no mask on the canvas). arc_kind_name and arc_ring_index map an occurrence's kind and ring
 *              to the name of its SHA tokens by comparing against the codes of flora.event_code.v1, never against a
 *              literal. The units that draw hold Godot types; this file holds none, so ase-adp-godot-arc-test runs
 *              without a Godot process.
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
#include <ase/adp/godot/godot_host_arc_view.hpp>
#include <ase/adp/godot/godot_host_clock_view.hpp>
#include <ase/adp/godot/godot_hud_arc_frame.hpp>

#include <cstdint>
#include <string>

namespace ase::adp::godot {

/** The stops of the fade across the frame's width: where each stands as a share of it and the opacity there. */
struct ArcFadeStops {
    double at[GODOT_ARC_FADE_STOPS] = {};     // hud_arc_fade_<n>: the place of the stop as a share of the frame's width
    double alpha[GODOT_ARC_FADE_STOPS] = {};  // hud_arc_fade_<n>_alpha: the opacity at that stop
};

/** Which port of the arc stands silent in a frame (the register N0448 logs each once on its edge). */
struct ArcGaps {
    bool arc = false;       // flora.arc.v1 carries no record
    bool almanac = false;   // flora.almanac.v1 carries no set of the arc's number
    bool codes = false;     // flora.event_code.v1 carries no complete set of codes
    bool unnamed = false;   // an occurrence or the next event names a kind or ring no code of the calendar names
};

/** The mockup's arc of SHA (hud_bridge_arc_*): what stands while a port carries no record. */
struct ArcMockup {
    double day_now = 0.0;     // hud_bridge_arc_day_now
    double lun_now = 0.0;     // hud_bridge_arc_lun_now
    double year_now = 0.0;    // hud_bridge_arc_year_now
    double next_in_h = 0.0;   // hud_bridge_arc_next_in_h
    double next_in_d = 0.0;   // hud_bridge_arc_next_in_d
    std::string next_kind;    // hud_bridge_arc_next_kind: a name of the register
};

/**
 * The frame of the arc from the three ports. The shares of now are those of flora.arc.v1; the occurrences those of the
 * held set of flora.almanac.v1, named by the codes of flora.event_code.v1. While the arc's number is not the held
 * set's, `paired` (the arc of the last frame whose number was the set's) stands instead, so anchor and occurrences
 * never come from two states. A silent port is answered in the gaps and the mockup's values stand for it.
 */
ArcGaps arc_bridge(const VivariumArcView& arc, const VivariumAlmanacView& almanac, const VivariumEventCodeView& codes,
                   const VivariumClockView& clock, const ArcMockup& mockup, VivariumArcView& paired, SkyArcFrame& out);

/** The angle, in degrees, a disc stands from the needle: (share of the occurrence minus share of now) times half_deg. */
double arc_disc_angle(double share, double now, double half_deg);

/** The angle `degrees` from the needle straight below a ring's centre, as UiRing::point takes it (from the top, clockwise). */
double arc_ring_angle(double degrees);

/** The opacity at `place` (a share of the frame's width): the line between the two stops it lies between, clamped outside. */
double arc_fade_opacity(const ArcFadeStops& stops, double place);

/** The name of the kind `code` (GODOT_ARC_KIND_*) by the codes of the calendar; nullptr when no code is it. */
const char* arc_kind_name(const VivariumEventCodeView& codes, double code);

/** The code of the kind `name` (GODOT_ARC_KIND_*) by the codes of the calendar; false when no kind has the name. */
bool arc_kind_code(const VivariumEventCodeView& codes, const char* name, double& code);

/** The slot of the ring `code` (GODOT_ARC_SLOT_*) by the codes of the calendar; false when no ring has it. */
bool arc_ring_index(const VivariumEventCodeView& codes, double code, uint32_t& index);

}  // namespace ase::adp::godot
