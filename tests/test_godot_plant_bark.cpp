/**
 * ASE ADAPTER TESTS - BARK OF THE INTERNODES WITHOUT GODOT
 *
 * @file        test_godot_plant_bark.cpp
 * @design      DSGN_024
 * @brief       Pins that the bark share and the bark tiling of an internode stay off the custom data of its instance
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2 and usecase:101613: the bark share of an internode
 *              (white bark against stem-base bark) and the tiling of its section reach the bark shader on CUSTOM0
 *              of every vertex of its tube (godot_tube_ring.cpp lay_tube_ring, pinned by
 *              test_godot_plant_tubes.cpp). The custom data beside an instance record carry the bend of a leaf card
 *              (PLAN_ASE_VIVARIUM_TREEGEN T.6, test_godot_plant_bend.cpp); an organ without a bend record, an
 *              internode among them, carries GODOT_BEND_NONE there, whatever its bark, so no shader of an instance
 *              reads a bark value as a bend.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100395 - age the bark
 * @actor       Birkenborke (Betula pendula)
 * @created     2026-10-26
 * @modified    2026-11-21
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_plant_records.hpp"

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_instances.hpp>
#include <ase/adp/godot/leaf/types.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstdint>

namespace ase::adp::godot {

using test_records::check_carries;
using test_records::instance;
using test_records::kPatch;
using test_records::organ;

TEST_CASE("usecase:100395 age the bark: the bark share of an internode stays off its instance, slot for slot flat") {
    VivariumOrganView stem = organ(11u, kPatch, 0.0);
    stem.bark = 0.75;  // the stem base, mostly stem-base bark
    VivariumOrganView band = organ(13u, kPatch, 0.0);
    band.bark = 0.25;  // the band towards the white bark
    const VivariumOrganView twig = organ(12u, kPatch, 3.0);  // a branch: white bark
    const auto split = rows_by_body({instance(11u), instance(12u), instance(13u)}, {stem, twig, band}, 2.0);
    const PlantBodyRows& plant = split.at(kPatch);

    // one custom slot per record, beside it in the same order, and no bark in any of them: no bend record, flat
    REQUIRE(plant.up_to_custom.size() == 2u * GODOT_INSTANCE_CUSTOM_VALUES);
    REQUIRE(plant.above_custom.size() == GODOT_INSTANCE_CUSTOM_VALUES);
    for (uint32_t v = 0u; v < GODOT_INSTANCE_CUSTOM_VALUES; ++v) {
        CHECK(plant.up_to_custom[v] == static_cast<float>(GODOT_BEND_NONE));
        CHECK(plant.up_to_custom[GODOT_INSTANCE_CUSTOM_VALUES + v] == static_cast<float>(GODOT_BEND_NONE));
        CHECK(plant.above_custom[v] == static_cast<float>(GODOT_BEND_NONE));
    }
    // the records themselves stay as the port carried them
    check_carries(plant.up_to, 0u, instance(11u));
    check_carries(plant.up_to, 1u, instance(13u));
    check_carries(plant.above, 0u, instance(12u));
}

TEST_CASE("03.11 step 2: the bark tiling of an internode stays off its instance, whatever its section carries") {
    VivariumOrganView stem = organ(11u, kPatch, 0.0);
    stem.bark = 0.5;
    stem.tile_around = 2.5;   // tiles of the white bark around the tube
    stem.tile_along = 0.75;   // tiles of the white bark along the tube
    stem.tile_base = 0.4;     // tiles of the stem-base bark per white tile
    const auto split = rows_by_body({instance(11u)}, {stem}, 2.0);
    const PlantBodyRows& plant = split.at(kPatch);

    REQUIRE(plant.up_to_custom.size() == GODOT_INSTANCE_CUSTOM_VALUES);
    for (uint32_t v = 0u; v < GODOT_INSTANCE_CUSTOM_VALUES; ++v) {
        CHECK(plant.up_to_custom[v] == static_cast<float>(GODOT_BEND_NONE));
    }
    // the view of the internode keeps its tiling for the tube, unchanged
    CHECK(stem.tile_around == doctest::Approx(2.5));
    CHECK(stem.tile_along == doctest::Approx(0.75));
    CHECK(stem.tile_base == doctest::Approx(0.4));
}

}  // namespace ase::adp::godot
