/**
 * ASE GODOT ADAPTER - VIVARIUM VIEW HUD SIGNS BRIDGE
 *
 * @file        godot_vivarium_hud_signs.cpp
 * @brief       VivariumView - the signs the bridge reports into the HUD's frame while no port reports a sign case
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD 06.10, design decision 2026-11-10: the HUD draws every element from
 *              its frame and never asks whether a port delivered. The patch set reports no state of a sign case yet
 *              (N0461) and flora.weather.v1 no weather signature (N0449), so the rail's dots, the markers at the
 *              birch and in the sky and the readings stand on the mockup's signs() applied to the values this
 *              frame already carries: the stage, condition and soil of the patch record, the season of the clock,
 *              the sky band's weather and night and the bridge's tap. Its thresholds and what hangs on the clock
 *              are hud_bridge_sign_* of SHA (theme.css --ase-hud-bridge-sign-*, VivariumSignRules): the
 *              adapter never judges the clock, the mockup's sky reports the evening red at its hour and its LUN is
 *              the LUN of rising sap. The values of the readings stand in full: the soil of the patch record, else
 *              the mockup's S.soil (hud_bridge_sign_soil_mm, _soil_rel), the clock's season, else the mockup's.
 *              Each gap is logged once on its edge with the port field named; nothing of it stands in the picture.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @design      DSGN_024
 * @category    ecs/module
 * @usecase     usecase:100370 - wilt from drought
 * @actor       Hänge-Birke (Betula pendula)
 * @register    N0461 (the state fields of each sign case), N0449 (the weather signatures), N0454, N0462, N0460,
 *              N0457 (the marks of the tools: soil_not_full, branch_uncut, season_saltatrix, tap_open), carrier N0448
 * @created     2026-11-10
 * @modified    2026-11-19
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <ase/adp/godot/godot_vivarium_node.hpp>
#include <ase/adp/godot/godot_hud_sign_marker.hpp>
#include <ase/adp/godot/types.hpp>

#include <ase/math/math.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot {

namespace {

/** `value` with `decimals` places as the mockup prints it (f1() with one, Math.round with none). */
std::string number_text(double value, int decimals) {
    return std::string(::godot::String::num(value, decimals).utf8().get_data());
}

/** The word texts.hud.seasons.<season> into `out`, its text runs joined; false where the theme names none. */
bool season_word(const UiTheme& theme, uint32_t season, std::string& out) {
    std::vector<HudRun> runs;
    out.clear();
    if (!theme.words("seasons." + std::to_string(season), runs)) {
        return false;
    }
    for (const HudRun& run : runs) {
        out += run.text;
    }
    return true;
}

}  // namespace

void VivariumView::bridge_signs(HudGameState& game, SignMarkerFrame& signs, const VivariumPatchView* patch,
                                const VivariumClockView& clock, const SkyBandFrame& band,
                                const HandSlotFrame& hand) {
    // THE EDGE OF THE BRIDGE FOR N0461 AND N0449: the reader of a port's signs is the first to stand; while the
    // patch set and flora.weather.v1 report none, the mockup's signs() decides over this frame's values.
    open_gap(GODOT_HUD_GAP_SIGN, "flora.patch.v1.signs");
    open_gap(GODOT_HUD_GAP_WEATHER, "flora.weather.v1.signatures");
    game.signs.clear();
    signs.values.clear();

    const bool dead = patch != nullptr && patch->dead;
    // The stage marks of the patch record against ase-plant's stage codes (usecase:101635, D2455): a stand stands
    // from the sprout on; without a patch in focus, or with a stage none of the codes names, nothing stands.
    const bool mature = patch != nullptr && patch->mature;
    const bool standing = !dead && patch != nullptr && (patch->sprout || patch->grow || patch->mature);
    // The soil of the patch record; without a record in focus the mockup's S.soil (hud_bridge_sign_soil_*), its
    // port field named.
    double soil_mm = hud_sign_rules_.soil_mm;
    double soil_rel = hud_sign_rules_.soil_rel;
    if (patch != nullptr) {
        soil_mm = patch->soil_mm;
        soil_rel = patch->soil_rel;
    } else {
        open_gap(GODOT_HUD_GAP_PATCH, "flora.patch.v1.soil_mm");
    }
    const auto season = static_cast<uint32_t>(ase::math::floor(static_cast<float>(clock.season)));
    // The season's signs compare against ase-calendar's codes as the date record carries them, never against a
    // number of the view; while the HUD's stand-in date stands, no code stands and no season's sign is shown.
    if (clock.season_codes_known) {
        close_gap(GODOT_HUD_GAP_CLOCK, "calendar.season_codes");
    } else {
        open_gap(GODOT_HUD_GAP_CLOCK, "calendar.season_codes");  // ase-calendar: CAL_CONST_SEASON_GLACIX .. HALITRIX
    }
    const bool glacix = clock.season_codes_known && season == static_cast<uint32_t>(clock.season_glacix);
    const bool saltatrix = clock.season_codes_known && season == static_cast<uint32_t>(clock.season_saltatrix);
    const bool halitrix = clock.season_codes_known && season == static_cast<uint32_t>(clock.season_halitrix);
    const std::string weather = band.weather_kind != nullptr ? band.weather_kind : "";
    const bool clear = weather == GODOT_HUD_WEATHER_CLEAR;
    const bool fog = weather == GODOT_HUD_WEATHER_FOG;

    // The mockup's signs() in its order: the death or the wilt, the season's sign of a standing birch, the catkins,
    // the tap or the rising sap, the brook in rain, and the three signatures of the sky.
    if (dead) {
        game.signs.emplace_back("dead");
    } else if (standing && soil_rel < hud_sign_rules_.wilt_soil) {
        game.signs.emplace_back("wilt");
    }
    if (standing && saltatrix) {
        game.signs.emplace_back("flush");
    }
    if (standing && halitrix) {
        game.signs.emplace_back("shed");
    }
    if (standing && glacix) {
        game.signs.emplace_back("rest");
    }
    if (!dead && saltatrix && mature) {
        game.signs.emplace_back("catkin");
    }
    if (hud_bridge_sap_running_) {
        game.signs.emplace_back("sap");
        if (hud_bridge_sap_l_ >= hud_bridge_sap_cap_l_) {
            game.marks.emplace_back(GODOT_HUD_MARK_VESSEL_FULL);
        }
    } else if (!dead && mature && hud_sign_rules_.sap_rise) {
        game.signs.emplace_back("sap_rise");
    }
    if (weather == GODOT_HUD_WEATHER_RAIN || weather == GODOT_HUD_WEATHER_DRIZZLE) {
        game.signs.emplace_back("brook");
    }
    if (clear && hud_sign_rules_.se02 && !band.night) {
        game.signs.emplace_back("se02");
    }
    if (clear && band.night) {
        game.signs.emplace_back("se30");
    }
    if (fog) {
        game.signs.emplace_back("se24");
    }

    // THE MARKS OF THE TOOLS: no port reckons the marks hud.tools requires beyond alive, dead and the stage, so
    // the mockup's TOOLS[].ok() decides over the same values: water while the soil has room (S.soil < 95),
    // prune while a candidate of the crown is uncut, the sap in SALTATRIX and while no tap runs, the plug while
    // one runs. Each gap is logged once on its edge with the port field named. The season's mark has no gap of its
    // own (N0460): it stands on clock.season, which flora.day.v1 carries, against CAL_CONST_SEASON_SALTATRIX as the
    // date record carries it (calendar.season_codes above); while no day record stands, VivariumView::bridge_clock
    // fills the season and logs flora.day.v1 itself.
    open_gap(GODOT_HUD_GAP_PATCH, "flora.patch.v1.soil_not_full");
    open_gap(GODOT_HUD_GAP_PRUNE, "flora.scene.v1.branch_uncut");
    open_gap(GODOT_HUD_GAP_SAP, "flora.patch.v1.tap_open");
    if (soil_rel < hud_bridge_soil_full_) {
        game.marks.emplace_back("soil_not_full");
    }
    // Prune's mark branch_uncut is laid in fill_hud_world, right after VivariumHandBridge::cut counted the crown:
    // fill_hud lays hud_now_ anew before this bridge, so hand.prune_left reads 0 here in every frame.
    if (saltatrix) {
        game.marks.emplace_back("season_saltatrix");
    }
    game.marks.emplace_back(hud_bridge_sap_running_ ? "tap_open" : "tap_closed");

    // The values the readings and bars name, each as the mockup prints it: f1() for millimetres and litres, a
    // rounded per cent for a share, the season's word, and the phases of rainIn() where the forecast names rain.
    // Every value stands: the clock's season where it names one of the mockup's seasons, else the mockup's season
    // (hud_bridge_clock_season), its port field named. load_hud_bridge holds a word for every season.
    signs.values.push_back(SignValue{"soil_mm", number_text(soil_mm, 1), 0.0});
    signs.values.push_back(SignValue{"soil_rel", number_text(soil_rel * 100.0, 0), soil_rel});
    uint32_t named = season;
    std::string word;
    if (!season_word(hud_.theme(), named, word) || named >= clock.season_light_h.size()) {
        open_gap(GODOT_HUD_GAP_CLOCK, "flora.day.v1.season");
        named = static_cast<uint32_t>(ase::math::floor(static_cast<float>(hud_bridge_clock_.season)));
        season_word(hud_.theme(), named, word);
    } else {
        close_gap(GODOT_HUD_GAP_CLOCK, "flora.day.v1.season");
    }
    const std::vector<double>& light = named < clock.season_light_h.size() ? clock.season_light_h
                                                                            : hud_bridge_clock_.season_light_h;
    signs.values.push_back(SignValue{"season", word, 0.0});
    signs.values.push_back(SignValue{"light_h", number_text(light[named], 0), 0.0});
    signs.values.push_back(SignValue{"lun", number_text(clock.month, 0), 0.0});
    signs.values.push_back(SignValue{"vessel_l", number_text(hud_bridge_sap_l_, 1), 0.0});
    signs.values.push_back(SignValue{"vessel_cap_l", number_text(hud_bridge_sap_cap_l_, 1), 0.0});
    const double vessel_rel = ase::math::min(1.0, hud_bridge_sap_l_ / hud_bridge_sap_cap_l_);
    signs.values.push_back(SignValue{"vessel_rel", number_text(vessel_rel * 100.0, 0), vessel_rel});
    signs.values.push_back(
        SignValue{"level_rel", number_text(hud_sign_rules_.brook_level * 100.0, 0), hud_sign_rules_.brook_level});
    double phases = hud_sign_rules_.se02_phases;
    if (hand.rain_coming) {
        phases = static_cast<double>(hand.rain_phases);
    } else if (fog) {
        phases = hud_sign_rules_.se24_phases;
    } else if (band.night) {
        phases = hud_sign_rules_.se30_phases;
    }
    signs.values.push_back(SignValue{"phases", number_text(phases, 0), 0.0});

    // The place of each carried item in the offer line: VivariumCarryBridge::fill writes the mockup's where() of
    // its inventory into signs.places while flora.carry.v1 reports no place (N0451).
    open_gap(GODOT_HUD_GAP_CARRY, "flora.carry.v1.where");
}

}  // namespace ase::adp::godot
