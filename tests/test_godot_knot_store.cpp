/**
 * ASE ADAPTER TESTS - WHEN THE VIEW READS THE KNOT PORTS AND WHEN A READ IS A NEW SET, WITHOUT GODOT
 *
 * @file        test_godot_knot_store.cpp
 * @design      DSGN_066
 * @brief       Pins the read rules of GodotKnotResourceManager: a new organ run owes a read, a differing read is a
 *              new set, an equal read ends the debt only once the passes cover the run and its pages and the knots
 *              name exactly the internodes of the segment port
 * @description PLAN_ASE_VIVARIUM_TREEGEN T.6 (usecase:101613 show the plant bark tubes, register N0543): ase-pl-flora
 *              files the knots, feet and cut faces page by page (GODOT_KNOT_ROWS_PER_PASS per GODOT_KNOT_SCHEDULE
 *              pass), and the host keeps the old set until the pass that files the last page. The manager decides
 *              from the counts alone whether the view lays the tubes anew, so these rules are checked here on plain
 *              views, before the host reader feeds it.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101613 - show the plant bark tubes
 * @actor       Worldkeeper Perception Relay (WORLPERREL)
 * @created     2026-11-21
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_host_organ_view.hpp>
#include <ase/adp/godot/tube/godot_knot_resource_manager.hpp>
#include <ase/adp/godot/tube/types.hpp>
#include <ase/adp/godot/port/types.hpp>

#include <cstdint>
#include <vector>

namespace ase::adp::godot {

namespace {

constexpr uint32_t kBody = 7u;    // object id of the patch the test plant stands on
constexpr uint32_t kStem = 11u;   // the first internode of the stem, the foot of its axis
constexpr uint32_t kUpper = 12u;  // its continuation child
constexpr uint32_t kOther = 13u;  // an internode the segment port shows and no knot of the held set names
constexpr uint64_t kRun = 1u;     // the organ run the first read follows
constexpr uint64_t kPass = 10u;   // the knot pass count at that run

/** The tip knot of one internode at a height, as the knot port shows it. */
VivariumKnotView knot_of(uint32_t internode, uint32_t below, double height) {
    VivariumKnotView knot;
    knot.object_id = internode;
    knot.body = kBody;
    knot.y = height;
    knot.radius = 0.05;
    knot.v = height;
    knot.below = below;
    return knot;
}

/** The two tip knots of the stem: the foot internode and its continuation. */
std::vector<VivariumKnotView> stem_knots(double top) {
    return {knot_of(kStem, kStem, 1.0), knot_of(kUpper, kStem, top)};
}

/** The foot knot of the stem axis. */
std::vector<VivariumFootView> stem_feet() {
    VivariumFootView foot;
    foot.object_id = kStem;
    foot.body = kBody;
    foot.radius = 0.06;
    foot.sides = 8u;
    return {foot};
}

/** The internodes of the segment port, one view per id. */
std::vector<VivariumOrganView> internodes_of(const std::vector<uint32_t>& ids) {
    std::vector<VivariumOrganView> views;
    for (uint32_t id : ids) {
        VivariumOrganView view;
        view.object_id = id;
        view.body = kBody;
        views.push_back(view);
    }
    return views;
}

/** The first pass count at which an equal read of a one-page set covers the run of kPass. */
uint64_t covered_pass() {
    return kPass + GODOT_ORGAN_READS_PER_RUN + 1u + 1u;
}

}  // namespace

TEST_CASE("show the plant bark tubes (N0543): a fresh manager owes a read, and the first set moves the generation") {
    GodotKnotResourceManager knots;
    CHECK(knots.get_generation() == 0u);
    CHECK(knots.due(kRun, kPass));

    knots.store_read(stem_knots(2.0), stem_feet(), {}, kPass, internodes_of({kStem, kUpper}));
    CHECK(knots.get_generation() == 1u);
    REQUIRE(knots.get_knots().size() == 2u);
    CHECK(knots.get_knots()[1].object_id == kUpper);
    CHECK(knots.get_feet().size() == 1u);
    CHECK(knots.get_scars().empty());
    CHECK_FALSE(knots.due(kRun, kPass + 1u));
}

TEST_CASE("show the plant bark tubes (N0543): an equal read after a new run ends the debt only once the passes cover "
          "the run and its page") {
    GodotKnotResourceManager knots;
    REQUIRE(knots.due(kRun, kPass));
    knots.store_read(stem_knots(2.0), stem_feet(), {}, kPass, internodes_of({kStem, kUpper}));
    REQUIRE(knots.get_generation() == 1u);

    // a new run owes a read again
    CHECK(knots.due(kRun + 1u, kPass));

    // the same set one pass later: the passes do not cover the reads of the run and the page yet
    knots.store_read(stem_knots(2.0), stem_feet(), {}, kPass + 1u, internodes_of({kStem, kUpper}));
    CHECK(knots.due(kRun + 1u, kPass + 1u));
    CHECK(knots.get_generation() == 1u);

    // covered and naming exactly the internodes of the segment port: the set stands, nothing is laid anew
    knots.store_read(stem_knots(2.0), stem_feet(), {}, covered_pass(), internodes_of({kStem, kUpper}));
    CHECK_FALSE(knots.due(kRun + 1u, covered_pass()));
    CHECK(knots.get_generation() == 1u);
}

TEST_CASE("show the plant bark tubes (N0543): an equal read that names other internodes is the old set and keeps the "
          "debt") {
    GodotKnotResourceManager knots;
    REQUIRE(knots.due(kRun, kPass));
    knots.store_read(stem_knots(2.0), stem_feet(), {}, kPass, internodes_of({kStem, kUpper}));
    REQUIRE(knots.due(kRun + 1u, kPass));

    // the segment port shows a third internode the held knots do not name: the new set is still being filed
    knots.store_read(stem_knots(2.0), stem_feet(), {}, covered_pass(), internodes_of({kStem, kUpper, kOther}));
    CHECK(knots.due(kRun + 1u, covered_pass()));
    CHECK(knots.get_generation() == 1u);
}

TEST_CASE("show the plant bark tubes (N0543): a differing read is the new set, and clear_all drops it") {
    GodotKnotResourceManager knots;
    REQUIRE(knots.due(kRun, kPass));
    knots.store_read(stem_knots(2.0), stem_feet(), {}, kPass, internodes_of({kStem, kUpper}));
    REQUIRE(knots.due(kRun + 1u, kPass));

    // the stem grew: its top knot stands higher, so the read differs and replaces the held set at once
    knots.store_read(stem_knots(2.5), stem_feet(), {}, kPass + 1u, internodes_of({kStem, kUpper}));
    CHECK(knots.get_generation() == 2u);
    REQUIRE(knots.get_knots().size() == 2u);
    CHECK(knots.get_knots()[1].y == doctest::Approx(2.5));
    CHECK_FALSE(knots.due(kRun + 1u, kPass + 1u));

    // a cut face makes a new set of all three ports
    knots.store_read(stem_knots(2.5), stem_feet(), {VivariumScarView{kUpper, kBody}}, kPass + 2u,
                     internodes_of({kStem, kUpper}));
    CHECK(knots.get_generation() == 3u);
    CHECK(knots.get_scars().size() == 1u);

    knots.clear_all();
    CHECK(knots.get_knots().empty());
    CHECK(knots.get_feet().empty());
    CHECK(knots.get_scars().empty());
    CHECK(knots.get_generation() == 4u);
}

}  // namespace ase::adp::godot
