/**
 * ASE GODOT ADAPTER - VIVARIUM HUD SIGN RULES
 *
 * @file        godot_vivarium_hud_sign_rules.cpp
 * @brief       The thresholds and values of the mockup's signs() the bridge reports by while no port reports a sign
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.10, see godot_vivarium_hud_sign_rules.hpp.
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
 * @version     00.00.01.00000
 */

#include <ase/adp/godot/godot_vivarium_hud_sign_rules.hpp>

#include <initializer_list>
#include <utility>

namespace ase::adp::godot {

bool VivariumSignRules::load(const UiTheme& theme, std::string& failed) {
    // The shares of the mockup's signs(): each lies in 0..1.
    for (const std::pair<const char*, double*>& share : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_bridge_sign_wilt_soil_rel", &wilt_soil},
             {"hud_bridge_sign_brook_level", &brook_level},
             {"hud_bridge_sign_soil_rel", &soil_rel}}) {
        if (!theme.measure(share.first, *share.second) || *share.second < 0.0 || *share.second > 1.0) {
            failed = std::string(share.first) + ": a share 0..1";
            return false;
        }
    }
    if (!theme.measure("hud_bridge_sign_soil_mm", soil_mm) || soil_mm < 0.0) {
        failed = "hud_bridge_sign_soil_mm: from 0";
        return false;
    }
    // What hangs on the clock: reported (1) or not (0), never judged by the adapter.
    double se02_reported = 0.0;
    double sap_rise_reported = 0.0;
    if (!theme.measure("hud_bridge_sign_se02_reported", se02_reported) ||
        !theme.measure("hud_bridge_sign_sap_rise_reported", sap_rise_reported) ||
        !(se02_reported == 0.0 || se02_reported == 1.0) || !(sap_rise_reported == 0.0 || sap_rise_reported == 1.0)) {
        failed = "hud_bridge_sign_se02_reported, _sap_rise_reported: 0 or 1";
        return false;
    }
    se02 = se02_reported == 1.0;
    sap_rise = sap_rise_reported == 1.0;
    // The phases the weather signatures promise: one at least.
    for (const std::pair<const char*, double*>& phases : std::initializer_list<std::pair<const char*, double*>>{
             {"hud_bridge_sign_se02_phases", &se02_phases},
             {"hud_bridge_sign_se30_phases", &se30_phases},
             {"hud_bridge_sign_se24_phases", &se24_phases}}) {
        if (!theme.measure(phases.first, *phases.second) || *phases.second < 1.0) {
            failed = std::string(phases.first) + ": from 1";
            return false;
        }
    }
    return true;
}

}  // namespace ase::adp::godot
