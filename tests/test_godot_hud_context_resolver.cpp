/**
 * ASE ADAPTER TESTS - THE BRAIN OF THE HUD WITHOUT GODOT
 *
 * @file        test_godot_hud_context_resolver.cpp
 * @design      DSGN_067
 * @brief       Pins the mode the HUD is in for a state: its hits, its layers, the lost sign, the lost tool, the level
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.2, DoD 06-C: the ContextResolver over all
 *              fourteen modes, the layers over every mode and the ones a mode leaves out (debug, drag,
 *              the open drawer), the fall back of a lost sign and of a tool whose item left the
 *              possession, the refusal of a rule or a state that names nothing, and the three levels
 *              (godot_hud_context_resolver.hpp). The rules are those of
 *              test_godot_hud_context_support.hpp; the fan and the signs stand in
 *              test_godot_hud_context_signs.cpp.
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

#include "test_godot_hud_context_support.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace ase::adp::godot::hud_test {

TEST_CASE("hud context resolver: the rules of the start configuration are accepted") {
    ContextResolver resolver;
    std::string failed;
    CHECK(resolver.set_spec(vivarium_spec(), failed));
    CHECK(resolver.spec().modes.size() == 14u);
    CHECK(resolver.spec().signs.size() == 12u);
}

TEST_CASE("hud context resolver: a rule that names nothing is refused with its key") {
    std::string failed;
    ContextResolver resolver;

    HudContextSpec spec = vivarium_spec();
    spec.start = "garden";
    CHECK_FALSE(resolver.set_spec(spec, failed));
    CHECK(failed == "hud.modes.start");

    spec = vivarium_spec();
    spec.sign_lost = "";
    CHECK_FALSE(resolver.set_spec(spec, failed));
    CHECK(failed == "hud.modes.sign_lost");

    spec = vivarium_spec();
    spec.tool_lost = "hand";
    CHECK_FALSE(resolver.set_spec(spec, failed));
    CHECK(failed == "hud.modes.tool_lost");

    spec = vivarium_spec();
    spec.signs[0].offer = "spade";
    CHECK_FALSE(resolver.set_spec(spec, failed));
    CHECK(failed == "hud.signs.dead.offer");

    spec = vivarium_spec();
    spec.signs[2].offer_when = {"stage_grow"};
    CHECK_FALSE(resolver.set_spec(spec, failed));
    CHECK(failed == "hud.signs.flush.offer_when");

    spec = vivarium_spec();
    spec.fan.push_back("sky");
    CHECK_FALSE(resolver.set_spec(spec, failed));
    CHECK(failed == "hud.focus.fan.tools");

    spec = vivarium_spec();
    spec.fan[5] = "water";
    CHECK_FALSE(resolver.set_spec(spec, failed));
    CHECK(failed == "hud.focus.fan.tools.water");

    spec = vivarium_spec();
    spec.fan[0] = "bucket";
    CHECK_FALSE(resolver.set_spec(spec, failed));
    CHECK(failed == "hud.focus.fan.tools.bucket");

    spec = vivarium_spec();
    spec.modes.push_back(spec.modes[1]);
    CHECK_FALSE(resolver.set_spec(spec, failed));
    CHECK(failed == "hud.modes.rest");

    spec = vivarium_spec();
    spec.over[0].marks.clear();
    CHECK_FALSE(resolver.set_spec(spec, failed));
    CHECK(failed == "hud.modes.over.debug");

    spec = vivarium_spec();
    spec.level_start = 4u;
    CHECK_FALSE(resolver.set_spec(spec, failed));
    CHECK(failed == "hud.perception.start");

    HudShown shown;
    CHECK_FALSE(resolver.resolve(HudGameState{}, shown, failed));
    CHECK(failed == "hud");
}

TEST_CASE("hud context resolver: every one of the fourteen modes shows its hits and its layers") {
    const ContextResolver resolver = ready_resolver();
    for (const HudModeSpec& mode : resolver.spec().modes) {
        CAPTURE(mode.name);
        HudGameState state = state_in(mode.name.c_str());
        state.possessed = every_item();  // a tool mode holds its item
        HudShown shown;
        std::string failed;
        REQUIRE(resolver.resolve(state, shown, failed));
        CHECK(resolver.spec().modes[shown.mode].name == mode.name);
        CHECK(shown.hits == mode.hits);
        CHECK(shown.layers == mode.layers);
    }
}

TEST_CASE("hud context resolver: the view starts in the rules' start mode, an empty or unknown mode is refused") {
    const ContextResolver resolver = ready_resolver();
    CHECK(resolver.spec().start == "arrive");
    HudShown shown;
    std::string failed;
    REQUIRE(resolver.resolve(state_in(resolver.spec().start.c_str()), shown, failed));
    CHECK(resolver.spec().modes[shown.mode].name == "arrive");

    CHECK_FALSE(resolver.resolve(state_in(""), shown, failed));
    CHECK(failed == "hud.modes.");

    HudGameState state = state_in("rest");
    state.mode = "garden";
    CHECK_FALSE(resolver.resolve(state, shown, failed));
    CHECK(failed == "hud.modes.garden");
}

TEST_CASE("hud context resolver: debug draws over every mode, the open drawer hides the inventory, drag draws last") {
    const ContextResolver resolver = ready_resolver();
    HudShown shown;
    std::string failed;

    HudGameState state = state_in("rest");
    state.marks = {"debug_on"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(shown.layers == Names{"band", "rail", "markers", "inventory", "debug"});

    state.marks = {"debug_on", "drawer_open"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(shown.layers == Names{"band", "rail", "markers", "debug"});

    state.marks = {"drawer_open"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(shown.layers == Names{"band", "rail", "markers", "inventory"});

    state.mode = "rack";
    state.marks = {"debug_on", "drawer_open", "dragging"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(shown.layers == Names{"band", "rack", "debug", "drag"});

    state.mode = "sky";
    state.marks = {"dragging"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(shown.layers == Names{"band", "sky", "drag"});
}

TEST_CASE("hud context resolver: the open sign is marked, a lost one falls back to rest") {
    const ContextResolver resolver = ready_resolver();
    HudShown shown;
    std::string failed;
    HudGameState state = state_in("sign");
    state.open_sign = "wilt";
    state.signs = {"dead", "wilt"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(resolver.spec().modes[shown.mode].name == "sign");
    REQUIRE(shown.open_sign == 1u);
    CHECK(shown.signs[1].open);
    CHECK_FALSE(shown.signs[0].open);

    state.signs = {"dead"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(resolver.spec().modes[shown.mode].name == "rest");
    CHECK(shown.open_sign == UINT32_MAX);
    CHECK(shown.layers == Names{"band", "rail", "markers", "inventory"});
}

TEST_CASE("hud context resolver: a tool runs only while its item is in possession, a lost one falls back to focus") {
    const ContextResolver resolver = ready_resolver();
    HudShown shown;
    std::string failed;
    for (const char* tool : {"water", "prune", "sow", "sap", "plug"}) {
        CAPTURE(tool);
        HudGameState state = state_in(tool);
        state.possessed = every_item();
        REQUIRE(resolver.resolve(state, shown, failed));
        CHECK(resolver.spec().modes[shown.mode].name == tool);

        state.possessed.clear();
        REQUIRE(resolver.resolve(state, shown, failed));
        CHECK(resolver.spec().modes[shown.mode].name == "focus");
        CHECK(shown.layers == Names{"band", "rail", "focus", "inventory"});
    }
    // a mode of no tool, or of a tool that needs no item, keeps its name without any possession
    for (const char* mode : {"terrain", "hold", "rack", "sky"}) {
        CAPTURE(mode);
        REQUIRE(resolver.resolve(state_in(mode), shown, failed));
        CHECK(resolver.spec().modes[shown.mode].name == mode);
    }
}

TEST_CASE("hud context resolver: the level is one of the three, never 0, and the rules start at 3") {
    const ContextResolver resolver = ready_resolver();
    CHECK(resolver.spec().level_start == 3u);
    HudShown shown;
    std::string failed;
    HudGameState state = state_in("rest");
    state.level = 0u;
    CHECK_FALSE(resolver.resolve(state, shown, failed));
    CHECK(failed == "hud.perception.levels");
    for (const uint32_t level : {1u, 2u, 3u}) {
        state.level = level;
        REQUIRE(resolver.resolve(state, shown, failed));
        CHECK(shown.level == level);
    }
    state.level = 4u;
    CHECK_FALSE(resolver.resolve(state, shown, failed));
    CHECK(failed == "hud.perception.levels");
}

}  // namespace ase::adp::godot::hud_test
