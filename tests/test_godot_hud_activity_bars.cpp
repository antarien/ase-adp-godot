/**
 * ASE ADAPTER TESTS - THE ACTIVITY BARS OF THE GAME HUD WITHOUT GODOT
 *
 * @file        test_godot_hud_activity_bars.cpp
 * @design      DSGN_074
 * @brief       Pins the order, the places, the lit sum, the strip and the hit of the activity bars
 * @description MIG_ASE_SKILL_RSM step B5: the youngest slice stands left and of two of the same age the later one, at
 *              most twenty bars show, a slice whose action, quantity or age is no number gets no bar, a bar is its
 *              quantity times the unit tall and never taller than the strip nor under its foot, the lit bars are those
 *              of the lit actions and their quantities add to the sum, the strip a finger reaches the bars on is the
 *              row widened by the pad, and a touch chooses the column under it, beyond the ends the first or the last.
 *              The unit knows no action code: the domain of a code is data (hud.bars of vivarium_start.json), so the
 *              codes of one case here are numbers no BDI_CONST_ACTION_* carries, on purpose, and the unit answers alike
 *              (as the arc test numbers the calendar's codes in an order of their own).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101305 - show skill progress
 * @actor       Skill Measurement Tracker (SKILMEATRA)
 * @created     2026-11-25
 * @modified    2026-11-25
 * @version     00.00.02.00002 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_hud_activity_bars.hpp>
#include <ase/adp/godot/rsm/types.hpp>

#include <cstdint>
#include <limits>
#include <vector>

namespace ase::adp::godot {

namespace {

constexpr double kNan = std::numeric_limits<double>::quiet_NaN();

/** The strip of theme.css --ase-hud-bars-*: x 231, foot 572, bars 2.4 wide with a gap of 1, 2 per quantity, 18 tall. */
ActivityBarsLayout layout_of() {
    ActivityBarsLayout layout;
    layout.x = 231.0;
    layout.base = 572.0;
    layout.w = 2.4;
    layout.gap = 1.0;
    layout.unit = 2.0;
    layout.h = 18.0;
    layout.pad = 4.0;
    return layout;
}

ActivityBar bar_of(uint32_t id, double action, double qnt, double age) {
    ActivityBar bar;
    bar.id = id;
    bar.action = action;
    bar.qnt = qnt;
    bar.age = age;
    return bar;
}

}  // namespace

TEST_CASE("activity bars: the youngest slice stands left, of two of the same age the later one") {
    const std::vector<ActivityBar> in = {bar_of(1u, 170.0, 1.0, 30.0), bar_of(2u, 169.0, 1.0, 5.0),
                                         bar_of(3u, 163.0, 1.0, 5.0), bar_of(4u, 164.0, 1.0, 12.0)};
    std::vector<ActivityBar> out;
    activity_bars_order(in, GODOT_RSM_BARS_MAX, out);
    REQUIRE(out.size() == 4u);
    CHECK(out[0].id == 3u);
    CHECK(out[1].id == 2u);
    CHECK(out[2].id == 4u);
    CHECK(out[3].id == 1u);
}

TEST_CASE("activity bars: at most twenty bars, the twenty youngest") {
    std::vector<ActivityBar> in;
    for (uint32_t index = 0; index < 25u; ++index) {
        in.push_back(bar_of(index, 163.0, 1.0, static_cast<double>(25u - index)));
    }
    std::vector<ActivityBar> out;
    activity_bars_order(in, GODOT_RSM_BARS_MAX, out);
    REQUIRE(out.size() == GODOT_RSM_BARS_MAX);
    CHECK(out.front().id == 24u);
    CHECK(out.back().id == 5u);
}

TEST_CASE("activity bars: a slice whose action, quantity or age is no number gets no bar") {
    const std::vector<ActivityBar> in = {bar_of(1u, kNan, 1.0, 1.0), bar_of(2u, 163.0, kNan, 1.0),
                                         bar_of(3u, 163.0, 1.0, kNan), bar_of(4u, -1.0, 1.0, 1.0),
                                         bar_of(5u, 168.0, 1.0, 1.0)};
    std::vector<ActivityBar> out;
    activity_bars_order(in, GODOT_RSM_BARS_MAX, out);
    REQUIRE(out.size() == 1u);
    CHECK(out[0].id == 5u);
}

TEST_CASE("activity bars: the unit knows no action code, codes no BDI_CONST_ACTION_* carries place alike") {
    const std::vector<ActivityBar> ordered = {bar_of(1u, 7.0, 2.0, 1.0), bar_of(2u, 4242.0, 1.0, 2.0),
                                              bar_of(3u, 7.0, 0.5, 3.0)};
    std::vector<ActivityBarPlace> places;
    activity_bars_place(ordered, layout_of(), {7u}, places);
    REQUIRE(places.size() == 3u);
    CHECK(places[0].action == 7u);
    CHECK(places[1].action == 4242u);
    CHECK(places[0].lit);
    CHECK_FALSE(places[1].lit);
    CHECK(places[2].lit);
    CHECK(activity_bars_lit_qnt(places) == doctest::Approx(2.5));
}

TEST_CASE("activity bars: a bar is its quantity times the unit tall, clamped to the strip, standing on its foot") {
    const ActivityBarsLayout layout = layout_of();
    const std::vector<ActivityBar> ordered = {bar_of(1u, 163.0, 3.0, 1.0), bar_of(2u, 169.0, 40.0, 2.0),
                                              bar_of(3u, 170.0, -2.0, 3.0)};
    std::vector<ActivityBarPlace> places;
    activity_bars_place(ordered, layout, {}, places);
    REQUIRE(places.size() == 3u);
    CHECK(places[0].action == 163u);
    CHECK(places[0].x == doctest::Approx(231.0));
    CHECK(places[0].h == doctest::Approx(6.0));
    CHECK(places[0].y == doctest::Approx(566.0));
    CHECK(places[1].x == doctest::Approx(234.4));
    CHECK(places[1].h == doctest::Approx(18.0));
    CHECK(places[1].y == doctest::Approx(554.0));
    CHECK(places[2].x == doctest::Approx(237.8));
    CHECK(places[2].h == doctest::Approx(0.0));
    CHECK(places[2].y == doctest::Approx(572.0));
    CHECK_FALSE(places[0].lit);
}

TEST_CASE("activity bars: the bars of the lit actions are lit and their quantities add to the sum") {
    const std::vector<ActivityBar> ordered = {bar_of(1u, 163.0, 3.0, 1.0), bar_of(2u, 169.0, 2.0, 2.0),
                                              bar_of(3u, 163.0, 1.5, 3.0), bar_of(4u, 170.0, 4.0, 4.0)};
    std::vector<ActivityBarPlace> places;
    activity_bars_place(ordered, layout_of(), {163u}, places);
    REQUIRE(places.size() == 4u);
    CHECK(places[0].lit);
    CHECK_FALSE(places[1].lit);
    CHECK(places[2].lit);
    CHECK_FALSE(places[3].lit);
    CHECK(activity_bars_lit_qnt(places) == doctest::Approx(4.5));

    activity_bars_place(ordered, layout_of(), {}, places);
    CHECK(activity_bars_lit_qnt(places) == doctest::Approx(0.0));
}

TEST_CASE("activity bars: the strip is the row widened by the pad, none without a bar") {
    const ActivityBarsLayout layout = layout_of();
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
    CHECK_FALSE(activity_bars_strip(layout, 0u, x, y, w, h));
    REQUIRE(activity_bars_strip(layout, 20u, x, y, w, h));
    CHECK(x == doctest::Approx(227.0));
    CHECK(y == doctest::Approx(550.0));
    CHECK(w == doctest::Approx(20.0 * 3.4 - 1.0 + 8.0));
    CHECK(h == doctest::Approx(26.0));
    // Twenty bars end at x 298, left of the put-away hit (x 300); the tallest stands under the hand's divider (y 552).
    CHECK(layout.x + 20.0 * 3.4 - 1.0 == doctest::Approx(298.0));
    CHECK(layout.base - layout.h >= 552.0);
}

TEST_CASE("activity bars: a touch chooses the column under it, beyond the ends the first or the last bar") {
    const ActivityBarsLayout layout = layout_of();
    const std::vector<ActivityBar> ordered = {bar_of(1u, 163.0, 1.0, 1.0), bar_of(2u, 169.0, 1.0, 2.0),
                                              bar_of(3u, 170.0, 1.0, 3.0)};
    std::vector<ActivityBarPlace> places;
    activity_bars_place(ordered, layout, {}, places);
    size_t index = 99u;
    REQUIRE(activity_bars_hit(places, layout, 231.0 + 3.4 + 0.5, 565.0, index));
    CHECK(index == 1u);
    REQUIRE(activity_bars_hit(places, layout, 228.0, 565.0, index));
    CHECK(index == 0u);
    REQUIRE(activity_bars_hit(places, layout, 231.0 + 3.0 * 3.4 + 2.0, 565.0, index));
    CHECK(index == 2u);
    // The gap right of a bar belongs to it.
    REQUIRE(activity_bars_hit(places, layout, 231.0 + 2.4 + 0.5, 565.0, index));
    CHECK(index == 0u);
    // Off the strip: left of the pad, under the foot past the pad, over the strip past the pad.
    CHECK_FALSE(activity_bars_hit(places, layout, 226.0, 565.0, index));
    CHECK_FALSE(activity_bars_hit(places, layout, 232.0, 577.0, index));
    CHECK_FALSE(activity_bars_hit(places, layout, 232.0, 549.0, index));
    // No bar, no hit.
    const std::vector<ActivityBarPlace> none;
    CHECK_FALSE(activity_bars_hit(none, layout, 232.0, 565.0, index));
}

TEST_CASE("activity band: the sign follows the slices and their quantities, never their age") {
    const std::vector<ActivityBar> first = {bar_of(7u, 155.0, 3.0, 1.0), bar_of(6u, 160.0, 2.0, 11.0)};
    std::vector<ActivityBar> older = first;
    older[0].age = 4.5;  // the running slice aged: no deed
    CHECK(activity_bars_sign(first) == activity_bars_sign(older));
    std::vector<ActivityBar> grown = first;
    grown[0].qnt = 4.0;  // the quantity of the running slice grew
    CHECK(activity_bars_sign(first) != activity_bars_sign(grown));
    std::vector<ActivityBar> sliced = first;
    sliced.insert(sliced.begin(), bar_of(8u, 155.0, 0.0, 0.0));  // a new slice began
    CHECK(activity_bars_sign(first) != activity_bars_sign(sliced));
}

namespace {

/** One frame of the band: the wall seconds, the sign of the bars, their count, the inventory in use, the switch. */
void band_frame(ActivityBandState& state, double wall, uint64_t sign, size_t count, bool needs_slots = false,
                bool toggled = false) {
    ActivityBandInput in;
    in.wall = wall;
    in.sign = sign;
    in.count = count;
    in.needs_slots = needs_slots;
    in.toggled = toggled;
    activity_band_step(in, GODOT_RSM_BAND_HOLD_SECONDS, state);
}

}  // namespace

TEST_CASE("activity band: the first set is no change, a change shows the bars, ten quiet seconds the slots") {
    ActivityBandState state;
    band_frame(state, 100.0, 11u, 3u);
    CHECK(state.shows == GODOT_RSM_BAND_SLOTS);  // the bars the port held at the start are the past
    band_frame(state, 100.5, 12u, 3u);
    CHECK(state.shows == GODOT_RSM_BAND_BARS);   // skilling: the quantity grew
    band_frame(state, 105.0, 12u, 3u);
    CHECK(state.shows == GODOT_RSM_BAND_BARS);   // 4.5 s of quiet: still the bars
    band_frame(state, 110.4, 12u, 3u);
    CHECK(state.shows == GODOT_RSM_BAND_BARS);   // 9.9 s
    band_frame(state, 110.5, 12u, 3u);
    CHECK(state.shows == GODOT_RSM_BAND_SLOTS);  // 10 s of quiet: the slots return
    band_frame(state, 111.0, 13u, 3u);
    CHECK(state.shows == GODOT_RSM_BAND_BARS);   // a new skilling turns it again
}

TEST_CASE("activity band: the inventory in use and the lack of bars show the slots at once") {
    ActivityBandState state;
    band_frame(state, 0.0, 1u, 2u);
    band_frame(state, 0.5, 2u, 2u);
    REQUIRE(state.shows == GODOT_RSM_BAND_BARS);
    band_frame(state, 1.0, 2u, 2u, true);
    CHECK(state.shows == GODOT_RSM_BAND_SLOTS);  // the pack opened or a thing is dragged
    band_frame(state, 1.5, 3u, 2u);
    CHECK(state.shows == GODOT_RSM_BAND_BARS);
    band_frame(state, 2.0, 4u, 0u);
    CHECK(state.shows == GODOT_RSM_BAND_SLOTS);  // no bar: nothing to show
    band_frame(state, 2.5, 4u, 0u, false, true);
    CHECK(state.shows == GODOT_RSM_BAND_SLOTS);  // without a bar the switch has nothing to turn to
    CHECK_FALSE(state.pinned);
}

TEST_CASE("activity band: the switch turns the band and holds it against the timer") {
    ActivityBandState state;
    band_frame(state, 0.0, 1u, 2u);
    band_frame(state, 1.0, 1u, 2u, false, true);
    CHECK(state.shows == GODOT_RSM_BAND_BARS);   // the switch on the slots: the bars, pinned
    CHECK(state.pinned);
    band_frame(state, 30.0, 1u, 2u);
    CHECK(state.shows == GODOT_RSM_BAND_BARS);   // the timer turns pinned bars no more
    band_frame(state, 31.0, 2u, 2u);
    CHECK(state.shows == GODOT_RSM_BAND_BARS);   // a change keeps pinned bars pinned
    CHECK(state.pinned);
    band_frame(state, 32.0, 2u, 2u, true);
    CHECK(state.shows == GODOT_RSM_BAND_SLOTS);  // the inventory in use still wins
    CHECK_FALSE(state.pinned);

    band_frame(state, 40.0, 3u, 2u);
    REQUIRE(state.shows == GODOT_RSM_BAND_BARS);
    band_frame(state, 41.0, 3u, 2u, false, true);
    CHECK(state.shows == GODOT_RSM_BAND_SLOTS);  // the switch on the bars: the slots, pinned
    CHECK(state.pinned);
    band_frame(state, 45.0, 4u, 2u);
    CHECK(state.shows == GODOT_RSM_BAND_SLOTS);  // a change without a quiet hold leaves the chosen slots
    band_frame(state, 54.0, 5u, 2u);
    CHECK(state.shows == GODOT_RSM_BAND_SLOTS);  // 9 s after the last change: still no quiet hold
    band_frame(state, 64.0, 6u, 2u);
    CHECK(state.shows == GODOT_RSM_BAND_BARS);   // a change after 10 s of quiet: a new skilling begins
    CHECK_FALSE(state.pinned);
}

}  // namespace ase::adp::godot
