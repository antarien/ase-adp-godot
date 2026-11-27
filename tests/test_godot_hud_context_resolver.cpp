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
 * @modified    2026-11-25
 * @version     00.00.22.00022 [refine]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_hud_context_support.hpp"

#include <ase/adp/godot/godot_hud_spine_pure.hpp>
#include <ase/fileio/path.hpp>
#include <ase/fileio/text_reader.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
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
    CHECK(shown.layers == Names{"ground_rows", "band", "rail", "body", "markers", "inventory", "debug"});

    state.marks = {"debug_on", "drawer_open"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(shown.layers == Names{"ground_rows", "band", "rail", "body", "markers", "debug"});

    state.marks = {"drawer_open"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(shown.layers == Names{"ground_rows", "band", "rail", "body", "markers", "inventory"});

    state.mode = "rack";
    state.marks = {"debug_on", "drawer_open", "dragging"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(shown.layers == Names{"ground_rows", "band", "rack", "debug", "drag"});

    state.mode = "sky";
    state.marks = {"dragging"};
    REQUIRE(resolver.resolve(state, shown, failed));
    CHECK(shown.layers == Names{"ground_rows", "band", "sky", "drag"});
}

// B0937 and the operator order of 2026-11-25: the body of the player with its survival bars stays on screen while a
// plant is looked at, and the rail is drawn before the card. The rail stands in rest, sign and focus; where the card's
// values end beside it is pinned by the case after this one.
TEST_CASE("hud context resolver: the plant card draws with the body rail before it") {
    const ContextResolver resolver = ready_resolver();
    const auto draws = [](const HudShown& shown, const char* name) {
        for (const std::string& layer : shown.layers) {
            if (layer == name) {
                return true;
            }
        }
        return false;
    };
    const auto rank = [](const HudShown& shown, const char* name) {
        uint32_t at = 0u;
        for (const std::string& layer : shown.layers) {
            if (layer == name) {
                return at;
            }
            ++at;
        }
        return at;
    };
    uint32_t rails = 0u;
    uint32_t cards = 0u;
    for (const HudModeSpec& mode : resolver.spec().modes) {
        CAPTURE(mode.name);
        HudGameState state = state_in(mode.name.c_str());
        state.possessed = every_item();
        HudShown shown;
        std::string failed;
        REQUIRE(resolver.resolve(state, shown, failed));
        rails += draws(shown, "body") ? 1u : 0u;
        cards += draws(shown, "focus") ? 1u : 0u;
        if (draws(shown, "focus")) {
            CHECK(draws(shown, "body"));
            CHECK(rank(shown, "body") < rank(shown, "focus"));
        }
    }
    CHECK(rails == 3u);  // rest, sign and focus
    CHECK(cards == 1u);  // focus
}

namespace {

// SHA's theme in the tree, from the adapter's folder: the file the stage turns into the tokens UiSpine reads
std::string theme_text() {
    const std::string adapter = ase::fileio::parent_of(ase::fileio::parent_of(__FILE__));
    return ase::fileio::read_text(
        ase::fileio::path_join(adapter, "../../clients/sha-client-web/sha-web-styles/src/theme.css"));
}

// the number of `--ase-<name>:` in the theme; found is false when the token is not there
double theme_token(const std::string& theme, const std::string& name, bool& found) {
    const std::string key = "--ase-" + name + ":";
    const std::size_t at = theme.find(key);
    found = at != std::string::npos;
    return found ? std::strtod(theme.c_str() + at + key.size(), nullptr) : 0.0;
}

}  // namespace

// B0937: the card's values are end-anchored at hud-focus-row-value-x, and the rail's line stands at the same x; the
// rail's content (track from hud-survival-rail-track-left to -right, the name end-anchored at -name-x, the number from
// -number-x on) lies left of its line, and rail row 1 and card row 0 share a band. No word of one is drawn over the
// other only when the card's values, and the bars under them, end at the rail's content while the rail draws.
TEST_CASE("hud context resolver: the values of the plant card end left of the body rail's content") {
    const std::string theme = theme_text();
    REQUIRE_MESSAGE(!theme.empty(), "clients/sha-client-web/sha-web-styles/src/theme.css");
    bool found = false;
    const double value_x = theme_token(theme, "hud-focus-row-value-x", found);
    REQUIRE(found);
    const double rail_x = theme_token(theme, "hud-survival-rail-x", found);
    REQUIRE(found);
    const double track_left = theme_token(theme, "hud-survival-rail-track-left", found);
    REQUIRE(found);
    const double track_right = theme_token(theme, "hud-survival-rail-track-right", found);
    REQUIRE(found);
    const double name_x = theme_token(theme, "hud-survival-rail-name-x", found);
    REQUIRE(found);
    const double number_x = theme_token(theme, "hud-survival-rail-number-x", found);
    REQUIRE(found);
    const double text_dx = theme_token(theme, "hud-focus-title-dx", found);
    REQUIRE(found);

    // the measured overlap: the content lies left of the line, and the card's own x reaches into it
    CHECK(track_left < track_right);
    CHECK(track_right <= rail_x);
    CHECK(track_left <= number_x);
    CHECK(name_x <= rail_x);
    CHECK(value_x > track_left);

    // without the rail the card keeps its own x
    CHECK(spine_value_end(value_x, false, track_left) == doctest::Approx(value_x));
    // with the rail it ends at the rail's content: left of the track, the number and the name
    const double end = spine_value_end(value_x, true, track_left);
    CHECK(end <= track_left);
    CHECK(end <= number_x);
    CHECK(end < name_x);
    CHECK(end <= value_x);

    // the bar under a value runs from the text column to the same end, and never backwards
    const double column = end - 2.0 * text_dx;
    CHECK(spine_bar_width(end, column) == doctest::Approx(end - column));
    CHECK(spine_bar_width(end, end + text_dx) == doctest::Approx(0.0));
}

// B1001: the labels start at the card's column and run right; a ring near the rail puts the column itself into the
// rail's content, and the value's end alone cannot keep a label off it. While the rail draws, the whole card yields
// left until its widest text (`need`, from the column to its right end) ends at hud-survival-rail-track-left.
TEST_CASE("hud context resolver: the labels of the plant card end left of the body rail's content") {
    const std::string theme = theme_text();
    REQUIRE_MESSAGE(!theme.empty(), "clients/sha-client-web/sha-web-styles/src/theme.css");
    bool found = false;
    const double track_left = theme_token(theme, "hud-survival-rail-track-left", found);
    REQUIRE(found);
    const double column_dx = theme_token(theme, "hud-focus-column-dx", found);
    REQUIRE(found);
    const double text_dx = theme_token(theme, "hud-focus-title-dx", found);
    REQUIRE(found);

    // a ring whose column stands inside the rail's content, and a card a label and a value wide
    const double near = track_left - column_dx + text_dx;
    const double column = near + column_dx;
    const double need = 4.0 * column_dx;
    REQUIRE(column + need > track_left);

    // without the rail the card keeps the column the ring gives it
    CHECK(spine_column_x(column, false, track_left, need) == doctest::Approx(column));
    // with it the card's right end comes to the rail's content, the labels with it
    const double yielded = spine_column_x(column, true, track_left, need);
    CHECK(yielded + need <= doctest::Approx(track_left));
    CHECK(yielded + text_dx < track_left);
    CHECK(yielded < column);

    // a card that already ends left of the rail's content never moves right
    const double far = track_left - 2.0 * need;
    CHECK(spine_column_x(far, true, track_left, need) == doctest::Approx(far));
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
    CHECK(shown.layers == Names{"ground_rows", "band", "rail", "body", "markers", "inventory"});
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
        CHECK(shown.layers == Names{"ground_rows", "band", "rail", "body", "focus", "inventory"});
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
