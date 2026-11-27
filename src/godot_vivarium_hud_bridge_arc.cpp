/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD ARC BRIDGE
 *
 * @file        godot_vivarium_hud_bridge_arc.cpp
 * @brief       VivariumView::bridge_arc and VivariumArcBridge - the frame of the sky arc, see godot_vivarium_hud_arc.hpp
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD_EXT package EXT.6, "Die Brücke". flora.arc.v1 is read in every frame, the
 *              almanac only when the arc's number differs from the set held (GodotHostResourceManager::read_arc), so
 *              the bridge holds the pair that agreed while the two disagree. A port without a record is a gap of the
 *              register N0448, logged once on its edge with the port named (open_gap) and closed on the edge its
 *              record stands (close_gap); the picture stays whole meanwhile.
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

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_vivarium_hud_arc.hpp>
#include <ase/adp/godot/godot_view_support.hpp>

#include <godot_cpp/variant/string.hpp>

#include <utility>

namespace ase::adp::godot {

bool VivariumArcBridge::load(const UiTheme& theme, std::string& failed) {
    if (!theme.measure("hud_bridge_arc_day_now", mockup_.day_now) ||
        !theme.measure("hud_bridge_arc_lun_now", mockup_.lun_now) ||
        !theme.measure("hud_bridge_arc_year_now", mockup_.year_now) ||
        !theme.measure("hud_bridge_arc_next_in_h", mockup_.next_in_h) ||
        !theme.measure("hud_bridge_arc_next_in_d", mockup_.next_in_d)) {
        failed = "hud_bridge_arc_day_now, _lun_now, _year_now, _next_in_h, _next_in_d";
        return false;
    }
    ::godot::String kind;
    if (!theme.token_text("hud_bridge_arc_next_kind", kind) || kind.is_empty()) {
        failed = "hud_bridge_arc_next_kind";
        return false;
    }
    mockup_.next_kind = utf8_of(kind);
    paired_ = VivariumArcView{};
    return true;
}

ArcGaps VivariumArcBridge::bridge(const VivariumArcView& arc, const VivariumAlmanacView& almanac,
                                  const VivariumEventCodeView& codes, const VivariumClockView& clock,
                                  SkyArcFrame& out) {
    return arc_bridge(arc, almanac, codes, clock, mockup_, paired_, out);
}

void VivariumView::bridge_arc(SkyArcFrame& arc, const VivariumClockView& clock) {
    // N0448: flora.arc.v1 every frame, flora.almanac.v1 for the set of the arc's number, flora.event_code.v1 for the
    // codes both are named with. A port that carries no record keeps the arc whole from SHA and is logged once.
    const ArcGaps gaps = hud_state_.hud_arc_.bridge(resources_.get_arc(), resources_.get_almanac(), resources_.get_event_codes(),
                                         clock, arc);
    const std::pair<bool, const char*> ports[] = {{gaps.arc, GODOT_ARC_GAP_ARC},
                                                  {gaps.almanac, GODOT_ARC_GAP_ALMANAC},
                                                  {gaps.codes, GODOT_ARC_GAP_CODES},
                                                  {gaps.unnamed, GODOT_ARC_GAP_UNKNOWN}};
    for (const std::pair<bool, const char*>& port : ports) {
        if (port.first) {
            open_gap(GODOT_HUD_GAP_ARC, port.second);
        } else {
            close_gap(GODOT_HUD_GAP_ARC, port.second);
        }
    }
    arc.elapsed = hud_state_.hud_clock_;
}

}  // namespace ase::adp::godot
