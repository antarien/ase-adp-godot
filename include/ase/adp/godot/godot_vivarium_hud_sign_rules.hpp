#pragma once

/**
 * ASE GODOT ADAPTER - VIVARIUM HUD SIGN RULES
 *
 * @file        godot_vivarium_hud_sign_rules.hpp
 * @brief       The thresholds and values of the mockup's signs() the bridge reports by while no port reports a sign
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.10, operator decision 2026-11-10: the HUD draws every element from
 *              its frame and never asks whether a port delivered. While the patch set reports no state of a sign
 *              case (N0461) and flora.weather.v1 no weather signature (N0449), VivariumView::bridge_signs
 *              (godot_vivarium_hud_signs.cpp) applies the mockup's signs() to the values the frame carries; the
 *              rules it applies are these, every one a hud_bridge_sign_* token of SHA (theme.css
 *              --ase-hud-bridge-sign-*).
 *
 *              wilt_soil is the soil share under which a standing plant wilts (S.soil < 25). brook_level is the
 *              brook's level (S.brook). The phases of steady weather the evening red promises (SE02), the phases to
 *              rain (SE30) and to fog (SE24) while the forecast names none. What hangs on the clock the adapter
 *              never judges: se02 holds while the mockup's sky reports the evening red at its hour, sap_rise while
 *              the mockup's LUN is the LUN of rising sap. soil_mm and soil_rel are the mockup's S.soil the
 *              readings name without a patch record in focus.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @category    ecs/module
 * @usecase     usecase:100370 - wilt from drought
 * @actor       Hänge-Birke (Betula pendula)
 * @register    N0461 (the state fields of each sign case), N0449 (the weather signatures), carrier N0448
 * @created     2026-11-11
 * @modified    2026-11-11
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_hud_ui_theme.hpp>

#include <string>

namespace ase::adp::godot {

/** The rules of the mockup's signs() while no port reports a sign case (N0461) or a weather signature (N0449). */
class VivariumSignRules {
public:
    /** Reads the hud_bridge_sign_* rules of SHA; false with the failing token and its range in `failed`. */
    bool load(const UiTheme& theme, std::string& failed);

    double wilt_soil = 0.0;    // hud_bridge_sign_wilt_soil_rel: the soil share under which a stand wilts
    bool se02 = false;         // hud_bridge_sign_se02_reported: the evening red the mockup's sky reports at its hour
    bool sap_rise = false;     // hud_bridge_sign_sap_rise_reported: the mockup's LUN is the LUN of rising sap
    double brook_level = 0.0;  // hud_bridge_sign_brook_level: the brook's level, a share
    double soil_mm = 0.0;      // hud_bridge_sign_soil_mm: the mockup's S.soil in millimetres without a patch record
    double soil_rel = 0.0;     // hud_bridge_sign_soil_rel: the same as a share of field capacity
    double se02_phases = 0.0;  // hud_bridge_sign_se02_phases: the phases of steady weather the evening red promises
    double se30_phases = 0.0;  // hud_bridge_sign_se30_phases: the phases to rain while the forecast names none
    double se24_phases = 0.0;  // hud_bridge_sign_se24_phases: the same for the fog
};

}  // namespace ase::adp::godot
