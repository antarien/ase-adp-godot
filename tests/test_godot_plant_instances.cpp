/**
 * ASE ADAPTER TESTS - INSTANCE RECORDS OF THE PLANT WITHOUT GODOT
 *
 * @file        test_godot_plant_instances.cpp
 * @design      DSGN_066
 * @brief       Pins that the instance records reach their body, the side of the order split and the bundle of their card unchanged
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2: rows_by_body sorts the records of an instance
 *              port by the body and the order their organ record names and hands the twelve values of each to
 *              the MultiMesh buffer as the port carried them (godot_plant_geometry.hpp). TREEGEN T.6: the card an
 *              organ record names (VivariumOrganView::card, the twelfth value of a leaf record) chooses the bundle,
 *              so a leaf in the far band reaches only the coarse bundle and a near leaf only the fine one.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101348 - hand organ instances to app
 * @actor       Organ Instance Courier (ORGNINST)
 * @created     2026-10-26
 * @modified    2026-11-24
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_plant_records.hpp"

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_instances.hpp>
#include <ase/adp/godot/port/types.hpp>
#include <ase/adp/godot/scene/types.hpp>
#include <ase/adp/godot/template/types.hpp>

#include <vector>

namespace ase::adp::godot {

using test_records::check_carries;
using test_records::instance;
using test_records::kOtherPatch;
using test_records::kPatch;
using test_records::organ;

TEST_CASE("03.11: the instance records go unchanged to the body and the side of the split their organ record names") {
    const std::vector<VivariumOrganView> organs = {
        organ(11u, kPatch, 0.0),       // the stem: up to the split
        organ(12u, kPatch, 3.0),       // a twig of order 3: above it
        organ(13u, kPatch, 2.0),       // order 2, the split itself: up to it
        organ(21u, kOtherPatch, 1.0)}; // the plant on the other patch
    const std::vector<VivariumInstanceView> instances = {instance(11u), instance(12u), instance(13u), instance(21u),
                                                         instance(99u)};  // 99 has no organ record
    const auto split = rows_by_body(instances, organs, 2.0, GODOT_TMPL_LEAF);
    REQUIRE(split.size() == 2u);
    // the twelve values exactly as the port carried them, record after record in port order
    const PlantBodyRows& plant = split.at(kPatch);
    REQUIRE(plant.up_to.size() == 2u * GODOT_INSTANCE_VALUES);
    REQUIRE(plant.above.size() == GODOT_INSTANCE_VALUES);
    check_carries(plant.up_to, 0u, instance(11u));
    check_carries(plant.up_to, 1u, instance(13u));
    check_carries(plant.above, 0u, instance(12u));
    const PlantBodyRows& other = split.at(kOtherPatch);
    check_carries(other.up_to, 0u, instance(21u));
    CHECK(other.above.empty());

    // a class the order budget does not split: every organ lies above GODOT_ORDER_EVERY, the stem too
    const auto every = rows_by_body(instances, organs, GODOT_ORDER_EVERY, GODOT_TMPL_LEAF);
    CHECK(every.at(kPatch).up_to.empty());
    REQUIRE(every.at(kPatch).above.size() == 3u * GODOT_INSTANCE_VALUES);
    check_carries(every.at(kPatch).above, 0u, instance(11u));
    check_carries(every.at(kPatch).above, 2u, instance(13u));

    // no records, no plants; records without organ records, no plants either
    CHECK(rows_by_body({}, organs, 2.0, GODOT_TMPL_LEAF).empty());
    CHECK(rows_by_body(instances, {}, 2.0, GODOT_TMPL_LEAF).empty());
}

TEST_CASE("TREEGEN T.6: the card of a leaf record chooses its bundle, the far leaf the coarse card, the near leaf the fine") {
    VivariumOrganView near_leaf = organ(11u, kPatch, 2.0);  // a body in the near band: the fine card of its file
    near_leaf.card = GODOT_TMPL_LEAF;
    VivariumOrganView far_leaf = organ(12u, kPatch, 2.0);   // a body in the far band: the coarse 1 x 1 card
    far_leaf.card = GODOT_TMPL_LEAF_CORS;
    VivariumOrganView far_other = organ(21u, kOtherPatch, 2.0);
    far_other.card = GODOT_TMPL_LEAF_CORS;
    const std::vector<VivariumOrganView> organs = {near_leaf, far_leaf, far_other};
    const std::vector<VivariumInstanceView> instances = {instance(11u), instance(12u), instance(21u)};

    // the fine bundle: only the near leaf, and only its body
    const auto fine = rows_by_body(instances, organs, GODOT_ORDER_EVERY, GODOT_TMPL_LEAF);
    REQUIRE(fine.size() == 1u);
    REQUIRE(fine.at(kPatch).above.size() == GODOT_INSTANCE_VALUES);
    check_carries(fine.at(kPatch).above, 0u, instance(11u));
    CHECK(fine.at(kPatch).above_custom.size() == GODOT_INSTANCE_CUSTOM_VALUES);
    CHECK(fine.at(kPatch).above_sway.size() == GODOT_SWAY_VALUES);

    // the coarse bundle: the two far leaves, each at its body, never the near one
    const auto coarse = rows_by_body(instances, organs, GODOT_ORDER_EVERY, GODOT_TMPL_LEAF_CORS);
    REQUIRE(coarse.size() == 2u);
    REQUIRE(coarse.at(kPatch).above.size() == GODOT_INSTANCE_VALUES);
    check_carries(coarse.at(kPatch).above, 0u, instance(12u));
    REQUIRE(coarse.at(kOtherPatch).above.size() == GODOT_INSTANCE_VALUES);
    check_carries(coarse.at(kOtherPatch).above, 0u, instance(21u));
    CHECK(coarse.at(kPatch).above_custom.size() == GODOT_INSTANCE_CUSTOM_VALUES);
    CHECK(coarse.at(kPatch).above_sway.size() == GODOT_SWAY_VALUES);

    // every record lands in exactly one bundle: the two bundles together carry all three
    CHECK(fine.at(kPatch).above.size() + coarse.at(kPatch).above.size() + coarse.at(kOtherPatch).above.size() ==
          3u * GODOT_INSTANCE_VALUES);
}

TEST_CASE("usecase:101335 animate the branch sway: every instance carries the sway of its organ unchanged, slot for slot") {
    VivariumOrganView swaying = organ(11u, kPatch, 0.0);
    swaying.sway_freq = 2.5;
    swaying.sway_mean_x = 0.125;
    swaying.sway_mean_z = -0.25;
    swaying.sway_gust_x = 0.0625;
    swaying.sway_gust_z = -0.125;
    swaying.sway_base = 0.25;
    swaying.sway_top = 0.75;
    const VivariumOrganView still = organ(12u, kPatch, 3.0);  // no sway record: the view stands it still
    const auto split = rows_by_body({instance(11u), instance(12u)}, {swaying, still}, 2.0, GODOT_TMPL_LEAF);
    const PlantBodyRows& plant = split.at(kPatch);

    // one sway slot per instance of its side, in the order of the data texture
    REQUIRE(plant.up_to_sway.size() == GODOT_SWAY_VALUES);
    REQUIRE(plant.above_sway.size() == GODOT_SWAY_VALUES);
    CHECK(plant.up_to_sway[0] == doctest::Approx(2.5));
    CHECK(plant.up_to_sway[1] == doctest::Approx(0.125));
    CHECK(plant.up_to_sway[2] == doctest::Approx(-0.25));
    CHECK(plant.up_to_sway[3] == doctest::Approx(0.0625));
    CHECK(plant.up_to_sway[4] == doctest::Approx(-0.125));
    CHECK(plant.up_to_sway[5] == doctest::Approx(0.25));
    CHECK(plant.up_to_sway[6] == doctest::Approx(0.75));
    for (uint32_t v = 0u; v < GODOT_SWAY_VALUES; ++v) {
        CHECK(plant.above_sway[v] == doctest::Approx(GODOT_SWAY_NONE));
    }
}

}  // namespace ase::adp::godot
