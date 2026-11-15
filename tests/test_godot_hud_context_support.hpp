#pragma once

/**
 * ASE ADAPTER TESTS - THE RULES OF THE HUD BRAIN THE CONTEXT TESTS SHARE
 *
 * @file        test_godot_hud_context_support.hpp
 * @design      DSGN_067
 * @brief       The rules of the start configuration as UiTheme fills them, and the states the two context tests start from
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.2, DoD 06-C. The ContextResolver is tested in two
 *              executables: test_godot_hud_context_resolver.cpp pins the modes, their hits and layers,
 *              the layers over every mode, the lost sign, the lost tool and the level;
 *              test_godot_hud_context_signs.cpp pins the fan, the tool conditions with and without
 *              possession, the twelve signs and their offers. Both apply the SAME rules - the sections
 *              hud.modes, hud.tools, hud.focus.fan, hud.signs and hud.perception of
 *              config/vivarium_start.json as UiTheme (godot_hud_ui_theme.cpp) reads them into a
 *              HudContextSpec - and these functions are that one shared part. Each executable takes its
 *              doctest entry point from its own target (tests/CMakeLists.txt), never from this header.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101440 - grow the interface with skills
 * @actor       Interface Growth Director (INTEGRODIR)
 * @created     2026-11-09
 * @modified    2026-11-09
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_hud_context_resolver.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot::hud_test {

using Names = std::vector<std::string>;

// hud.modes, hud.tools, hud.focus.fan, hud.signs and hud.perception of the start configuration
inline HudContextSpec vivarium_spec() {
    HudContextSpec spec;
    spec.modes = {
        {"arrive", {}, {"band", "arrive"}},
        {"rest", {"world", "tree", "ground"}, {"band", "rail", "markers", "inventory"}},
        {"sign", {"world", "tree", "ground"}, {"band", "rail", "markers", "sign_open", "inventory"}},
        {"focus", {"world", "tree", "ground"}, {"band", "rail", "focus", "inventory"}},
        {"water", {"world", "ground"}, {"band", "inventory"}},
        {"prune", {"world", "ground"}, {"band", "prune_marks", "inventory"}},
        {"sow", {"world", "ground"}, {"band", "inventory"}},
        {"terrain", {"world", "ground"}, {"band", "reticle", "inventory"}},
        {"sap", {"world", "trunk", "ground"}, {"band", "inventory"}},
        {"plug", {"world", "trunk", "ground"}, {"band", "inventory"}},
        {"hold", {"world", "tree", "ground"}, {"band", "inventory"}},
        {"rack", {}, {"band", "rack", "inventory"}},
        {"sky", {}, {"band", "sky"}},
        {"sys", {}, {"band", "system"}},
    };
    spec.over = {{"debug", {"debug_on"}}, {"drag", {"dragging"}}};
    spec.hides = {{"inventory", {"debug_on", "drawer_open"}}};
    spec.start = "arrive";
    spec.sign_lost = "rest";
    spec.tool_lost = "focus";
    spec.tools = {
        {"water", {"alive", "soil_not_full"}, "regador"},
        {"prune", {"alive", "stage_grow", "branch_uncut"}, "podao"},
        {"sow", {"dead"}, "plantador"},
        {"sap", {"alive", "stage_mature", "season_saltatrix", "tap_closed"}, "trado"},
        {"plug", {"tap_open"}, "cavilha"},
        {"ground", {}, ""},
        {"sky", {}, ""},
    };
    spec.fan = {"water", "prune", "sow", "sap", "plug", "ground"};
    spec.fan_places = 6u;
    spec.signs = {
        {"dead", true, "", "sow", {}},
        {"wilt", true, "", "water", {}},
        {"flush", false, "", "", {}},
        {"shed", false, "", "prune", {"stage_grow"}},
        {"rest", false, "", "", {}},
        {"catkin", false, "", "", {}},
        {"sap", false, "vessel_full", "plug", {}},
        {"sap_rise", false, "", "sap", {}},
        {"brook", false, "", "ground", {}},
        {"se02", false, "", "sky", {}},
        {"se30", false, "", "sky", {}},
        {"se24", false, "", "sky", {}},
    };
    spec.levels = {1u, 2u, 3u};
    spec.level_start = 3u;
    return spec;
}

// the items of the five item tools: the watering pot, the billhook, the setting stick, the auger, the peg
inline Names every_item() {
    return {"regador", "podao", "plantador", "trado", "cavilha"};
}

inline ContextResolver ready_resolver() {
    ContextResolver resolver;
    std::string failed;
    REQUIRE(resolver.set_spec(vivarium_spec(), failed));
    return resolver;
}

// the names of the fan's tools in the order of their places
inline Names fan_names(const ContextResolver& resolver, const HudShown& shown) {
    Names names;
    for (uint32_t index = 0; index < shown.fan.size(); ++index) {
        CHECK(shown.fan[index].place == index);
        names.push_back(resolver.spec().tools[shown.fan[index].tool].name);
    }
    return names;
}

// a state in `mode` at the start level of the rules (hud.perception.start), as the view sets it
inline HudGameState state_in(const char* mode) {
    HudGameState state;
    state.mode = mode;
    state.level = 3u;
    return state;
}

// every mark the tools of the fan require, and every item they need
inline HudGameState everything_held() {
    HudGameState state = state_in("focus");
    state.place_selected = true;
    state.marks = {"alive", "soil_not_full", "stage_grow", "branch_uncut", "stage_mature", "season_saltatrix",
                   "tap_closed", "dead", "tap_open"};
    state.possessed = every_item();
    return state;
}

}  // namespace ase::adp::godot::hud_test
