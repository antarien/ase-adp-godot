/**
 * ASE ADAPTER TESTS - THE FAN AND THE SIGNS OF THE HUD WITHOUT GODOT
 *
 * @file        test_godot_hud_context_signs.cpp
 * @design      DSGN_067
 * @brief       Pins which tools the fan carries and which signs, alerts and offers the HUD shows for a state
 * @description PLAN_ASE_VIVARIUM_PHASE_06_HUD package 06.2, DoD 06-C: the tool conditions with and without
 *              possession, the places of the fan, the twelve signs in the order of the rail, their alerts
 *              and their offers (godot_hud_context_resolver.hpp). The rules are those of
 *              test_godot_hud_context_support.hpp; the modes and the level stand in
 *              test_godot_hud_context_resolver.cpp.
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

TEST_CASE("hud context resolver: the fan shows only while a place is selected and never the sky") {
    const ContextResolver resolver = ready_resolver();
    HudShown shown;
    std::string failed;

    HudGameState state = everything_held();
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(fan_names(resolver, shown) == Names{"water", "prune", "sow", "sap", "plug", "ground"});

    state.place_selected = false;
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(shown.fan.empty());
}

TEST_CASE("hud context resolver: each of the five item tools needs all of its marks and its item") {
    const ContextResolver resolver = ready_resolver();
    struct ToolCase {
        const char* tool;
        Names marks;
        const char* item;
    };
    const std::vector<ToolCase> cases = {
        {"water", {"alive", "soil_not_full"}, "regador"},
        {"prune", {"alive", "stage_grow", "branch_uncut"}, "podao"},
        {"sow", {"dead"}, "plantador"},
        {"sap", {"alive", "stage_mature", "season_saltatrix", "tap_closed"}, "trado"},
        {"plug", {"tap_open"}, "cavilha"},
    };
    for (const ToolCase& tool_case : cases) {
        CAPTURE(tool_case.tool);
        HudShown shown;
        std::string failed;
        HudGameState state = state_in("focus");
        state.place_selected = true;
        state.marks = tool_case.marks;
        state.possessed = {tool_case.item};
        REQUIRE(resolver.resolve(state, shown, failed));
        CHECK(fan_names(resolver, shown) == Names{tool_case.tool, "ground"});

        state.possessed.clear();
        REQUIRE(resolver.resolve(state, shown, failed));
        CHECK(fan_names(resolver, shown) == Names{"ground"});

        for (uint32_t dropped = 0; dropped < tool_case.marks.size(); ++dropped) {
            CAPTURE(tool_case.marks[dropped]);
            state.possessed = {tool_case.item};
            state.marks = tool_case.marks;
            state.marks.erase(state.marks.begin() + dropped);
            REQUIRE(resolver.resolve(state, shown, failed));
            CHECK(fan_names(resolver, shown) == Names{"ground"});
        }
    }
}

TEST_CASE("hud context resolver: the ground tools stand at the tool place and need neither mark nor item") {
    const ContextResolver resolver = ready_resolver();
    HudShown shown;
    std::string failed;
    HudGameState state = state_in("focus");
    state.place_selected = true;
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(fan_names(resolver, shown) == Names{"ground"});
}

TEST_CASE("hud context resolver: the places of the fan close up behind a tool that does not show") {
    const ContextResolver resolver = ready_resolver();
    HudShown shown;
    std::string failed;
    HudGameState state = everything_held();
    state.possessed = {"podao", "cavilha"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(fan_names(resolver, shown) == Names{"prune", "plug", "ground"});
}

TEST_CASE("hud context resolver: all twelve signs show in the order of the rail, whatever order the ports report") {
    const ContextResolver resolver = ready_resolver();
    HudShown shown;
    std::string failed;
    HudGameState state = state_in("rest");
    state.signs = {"se24", "se30", "se02", "brook", "sap_rise", "sap", "catkin", "rest", "shed", "flush", "wilt", "dead"};
    REQUIRE(resolver.resolve(state, shown, failed));
    REQUIRE(shown.signs.size() == 12u);
    for (uint32_t index = 0; index < 12u; ++index) {
        CHECK(shown.signs[index].sign == index);
        CHECK_FALSE(shown.signs[index].open);
    }
    CHECK(shown.open_sign == UINT32_MAX);
    CHECK(shown.alerts == 2u);
    CHECK(shown.signs[0].alert);
    CHECK(shown.signs[1].alert);
    for (uint32_t index = 2; index < 12u; ++index) {
        CHECK_FALSE(shown.signs[index].alert);
    }
}

TEST_CASE("hud context resolver: each sign offers its tool, or nothing") {
    const ContextResolver resolver = ready_resolver();
    HudShown shown;
    std::string failed;
    HudGameState state = state_in("rest");
    state.marks = {"stage_grow"};
    state.signs = {"dead", "wilt", "flush", "shed", "rest", "catkin", "sap", "sap_rise", "brook", "se02", "se30", "se24"};
    REQUIRE(resolver.resolve(state, shown, failed));
    REQUIRE(shown.signs.size() == 12u);
    const std::vector<const char*> offers = {"sow", "water", "", "prune", "", "", "plug", "sap", "ground", "sky", "sky", "sky"};
    for (uint32_t index = 0; index < 12u; ++index) {
        CAPTURE(resolver.spec().signs[index].name);
        const std::string offer = offers[index];
        if (offer.empty()) {
            CHECK(shown.signs[index].offer == UINT32_MAX);
            CHECK_FALSE(shown.signs[index].offer_got);
        } else {
            REQUIRE(shown.signs[index].offer != UINT32_MAX);
            CHECK(resolver.spec().tools[shown.signs[index].offer].name == offer);
        }
    }
}

TEST_CASE("hud context resolver: an offer shows without its item, flagged as missing") {
    const ContextResolver resolver = ready_resolver();
    HudShown shown;
    std::string failed;
    HudGameState state = state_in("rest");
    state.signs = {"wilt", "dead", "sap", "sap_rise", "brook", "se02"};
    REQUIRE(resolver.resolve(state, shown, failed));
    REQUIRE(shown.signs.size() == 6u);
    CHECK_FALSE(shown.signs[0].offer_got);  // dead offers the setting stick
    CHECK_FALSE(shown.signs[1].offer_got);  // wilt offers the watering pot
    CHECK_FALSE(shown.signs[2].offer_got);  // sap offers the peg
    CHECK_FALSE(shown.signs[3].offer_got);  // sap_rise offers the auger
    CHECK(shown.signs[4].offer_got);        // brook offers the ground tools, which need no item
    CHECK(shown.signs[5].offer_got);        // se02 offers the sky, which needs no item

    state.possessed = {"regador", "plantador", "cavilha", "trado"};
    REQUIRE(resolver.resolve(state, shown, failed));
    for (const HudSignShown& sign : shown.signs) {
        CHECK(sign.offer_got);
    }
}

TEST_CASE("hud context resolver: shed offers the billhook only from Crescendo on") {
    const ContextResolver resolver = ready_resolver();
    HudShown shown;
    std::string failed;
    HudGameState state = state_in("rest");
    state.signs = {"shed"};
    REQUIRE(resolver.resolve(state, shown, failed));
    REQUIRE(shown.signs.size() == 1u);
    CHECK(shown.signs[0].offer == UINT32_MAX);

    state.marks = {"stage_grow"};
    REQUIRE(resolver.resolve(state, shown, failed));
    REQUIRE(shown.signs[0].offer != UINT32_MAX);
    CHECK(resolver.spec().tools[shown.signs[0].offer].name == "prune");
}

TEST_CASE("hud context resolver: the sap sign alerts only while its vessel is full") {
    const ContextResolver resolver = ready_resolver();
    HudShown shown;
    std::string failed;
    HudGameState state = state_in("rest");
    state.signs = {"sap"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK_FALSE(shown.signs[0].alert);
    CHECK(shown.alerts == 0u);

    state.marks = {"vessel_full"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(shown.signs[0].alert);
    CHECK(shown.alerts == 1u);
}

TEST_CASE("hud context resolver: a sign no rule knows is refused") {
    const ContextResolver resolver = ready_resolver();
    HudShown shown;
    std::string failed;
    HudGameState state = state_in("rest");
    state.signs = {"wilt", "frost"};
    CHECK_FALSE(resolver.resolve(state, shown, failed));
    CHECK(failed == "hud.signs.frost");
}

}  // namespace ase::adp::godot::hud_test
