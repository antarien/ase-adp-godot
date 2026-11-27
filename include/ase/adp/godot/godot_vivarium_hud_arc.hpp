#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD ARC BRIDGE
 *
 * @file        godot_vivarium_hud_arc.hpp
 * @brief       VivariumArcBridge - the frame of the sky arc from the three arc ports, the mockup's while they are silent
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, "Die Brücke". bridge fills SkyArcFrame every frame:
 *              the shares of now from flora.arc.v1 (VivariumArcView, read in every frame and exempt from
 *              hud_refresh_ms), the occurrences from flora.almanac.v1 (VivariumAlmanacView) named and sorted onto their
 *              ring by the codes of flora.event_code.v1 (VivariumEventCodeView), never by a number of its own, and the
 *              SURA date from the date record of the clock. The rule itself is arc_bridge (godot_hud_arc_pure.hpp),
 *              without a Godot type, so the host test holds it; this class carries SHA's mockup values and the pair
 *              that agreed.
 *
 *              ANCHOR AND OCCURRENCES NEVER COME FROM TWO STATES. The shares of now are shares against the anchor of a
 *              set, the occurrences are shares against the same anchor: while the arc's number (seq) is not the
 *              number of the set held, the frame keeps the last pair that agreed (paired_) and shows no mixture.
 *
 *              THE GAP. Until a port carries its first record the arc stands whole with the mockup's values
 *              (hud_bridge_arc_day_now, _lun_now, _year_now, _next_kind, _next_in_h of SHA, laid at "now", so each
 *              ring's share of now is its anchor 0); the rings then hold their tracks and ticks and no signs. bridge
 *              answers which port is silent (ArcGaps); VivariumView::bridge_arc logs each once on its edge (open_gap,
 *              register N0448) and closes it on the edge the port's record stands (E12).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_066
 * @category    ecs/module
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @register    N0448 (flora.arc.v1, flora.almanac.v1, flora.event_code.v1)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.27.00027 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_arc_pure.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <string>

namespace ase::adp::godot {

/** The bridge of the sky arc. */
class VivariumArcBridge {
public:
    /** Reads hud_bridge_arc_* of SHA; false with the missing name in `failed`. */
    bool load(const UiTheme& theme, std::string& failed);

    /** Fills `out` from the ports, the mockup's values where a port is silent; answers the silent ports. */
    ArcGaps bridge(const VivariumArcView& arc, const VivariumAlmanacView& almanac, const VivariumEventCodeView& codes,
                   const VivariumClockView& clock, SkyArcFrame& out);

private:
    ArcMockup mockup_;           // hud_bridge_arc_* of SHA
    VivariumArcView paired_;     // the arc of the last frame whose number was the number of the set held
};

}  // namespace ase::adp::godot
