#pragma once

/**
 * ASE GODOT ADAPTER - HUD SURVIVAL VIEW
 *
 * @file        godot_hud_survival_view.hpp
 * @brief       SurvivalView: the eight survival marks of the HUD, bound, set and drawn as one part of the composition
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD section survival, rows U01 to U08 (mockup VIVARIUM_PHASE_08_SURVIVAL_HUD_V1,
 *              s.html 187 to 239). The survival marks are one part of the HUD: they read the body, the fire, the rest,
 *              the air and the food the bridges wrote into the frame, and they draw on two layers of hud.modes:
 *
 *                rail     SurvivalRailStates: the states of the body under the signs on the rail SINAIS (U02,
 *                         s.html 187), on the same wall clock as the rail's fade
 *                body     in the order render() lays them (s.html 239): SurvivalBodyRail, the rail CORPO at the right
 *                         edge (U01); SurvivalWarmthScale, the scale CONFORTO over its heat row (U03); SurvivalFireCard,
 *                         the card of the camp fire (U04); SurvivalRestCard, the card of the resting figure (U05);
 *                         SurvivalAirMark, the mark AR TÓXICO (U06); SurvivalSpoilRing, the ring ESTRAGO of the bag's
 *                         food (U07); SurvivalWaterMark, the drop at the bag (U08), last
 *
 *              VivariumHud holds one SurvivalView, binds it with the other blocks and hands it the frame's survival
 *              records on these two layers; every value is the owner's, written by the bridges, never made here.
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
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_survival_air_mark.hpp>
#include <ase/adp/godot/godot_hud_survival_body_rail.hpp>
#include <ase/adp/godot/godot_hud_survival_fire_card.hpp>
#include <ase/adp/godot/godot_hud_survival_rail_states.hpp>
#include <ase/adp/godot/godot_hud_survival_rest_card.hpp>
#include <ase/adp/godot/godot_hud_survival_spoil_ring.hpp>
#include <ase/adp/godot/godot_hud_survival_warmth_scale.hpp>
#include <ase/adp/godot/godot_hud_survival_water_mark.hpp>
#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <ase/adp/godot/godot_hud_ui_pen_decl.hpp>
#include <godot_cpp/variant/transform2d.hpp>

#include <string>

namespace ase::adp::godot {

/** The survival records of one frame, as the bridges wrote them into VivariumHudFrame. */
struct SurvivalViewFrame {
    const SurvivalBodyFrame* body = nullptr;   // REQUIRED: the figure's body (U01, U02, U03, U08)
    const SurvivalFireFrame* fire = nullptr;   // REQUIRED on the layer body: the camp fire (U04)
    const SurvivalRestFrame* rest = nullptr;   // REQUIRED on the layer body: the figure's rest (U05)
    const SurvivalAirFrame* air = nullptr;     // REQUIRED on the layer body: the rating of the air (U06)
    const SurvivalSpoilFrame* food = nullptr;  // REQUIRED on the layer body: the spoil of the bag's food (U07)
    double elapsed = 0.0;                      // the wall seconds since the view appeared: the fades play on them
    bool still = false;                        // every motion held
};

/** The eight survival marks of the HUD. */
class SurvivalView {
public:
    /** Binds all eight marks from `theme`; false with the missing name. */
    bool bind(const UiTheme& theme, std::string& failed);

    /** The layer rail: the states of the body under the signs (U02); false with the reason. */
    bool draw_states(UiPen& canvas, const ::godot::Transform2D& frame, const SurvivalViewFrame& now,
                     std::string& failed);

    /** The layer body: the rail CORPO and the marks that hang on it, in the order of render(); false with the reason. */
    bool draw_body(UiPen& canvas, const ::godot::Transform2D& frame, const SurvivalViewFrame& now,
                   std::string& failed);

private:
    SurvivalBodyRail body_;     // the body rail CORPO at the right edge (U01, mockup VIVARIUM_PHASE_08 S1)
    SurvivalWarmthScale warmth_;// the scale CONFORTO over the rail's heat row (U03, mockup VIVARIUM_PHASE_08 S3)
    SurvivalRailStates states_; // the states of the body under the signs on the rail SINAIS (U02, mockup VIVARIUM_PHASE_08 S2)
    SurvivalFireCard fire_;     // the card of the camp fire on its leader from the hearth (U04, mockup VIVARIUM_PHASE_08 S4)
    SurvivalRestCard rest_;     // the card of the resting figure on its leader from the figure (U05, mockup VIVARIUM_PHASE_08 S5)
    SurvivalAirMark air_;       // the mark AR TÓXICO in the sky band under the wind (U06, mockup VIVARIUM_PHASE_08 S6)
    SurvivalSpoilRing food_;    // the ring ESTRAGO around the food of the bag (U07, mockup VIVARIUM_PHASE_08 S7)
    SurvivalWaterMark water_;   // the drop at the bag, ÁGUA TURVA while poisoned (U08, mockup VIVARIUM_PHASE_08 S8)
};

}  // namespace ase::adp::godot
