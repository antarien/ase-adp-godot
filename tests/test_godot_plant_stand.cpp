/**
 * ASE ADAPTER TESTS - CONDITION OF THE BED WITHOUT GODOT
 *
 * @file        test_godot_plant_stand.cpp
 * @design      DSGN_066
 * @brief       Pins that soil water and condition of a patch reach its materials as the record carried them
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.2: plant_stands takes the soil water over field capacity and
 *              the condition code of every patch record, and a value the record did not carry stays unknown, so
 *              the material draws its shader's default, never a value made up (godot_plant_geometry.hpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101340 - show the bed condition
 * @actor       Care View Supplier (CARESUPPLY)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.21.00021 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/types.hpp>

#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

namespace {

constexpr uint32_t kPatch = 7u;       // object id of a measured patch
constexpr uint32_t kOtherPatch = 8u;  // object id of a patch before the first runs of the modules

}  // namespace

TEST_CASE("03.2: soil water and condition of a patch reach its materials as the record carried them") {
    // a measured stand: soil water over field capacity and the wilted mark read_snapshot set against flora.cond.v1
    VivariumPatchView patch;
    patch.object_id = kPatch;
    patch.soil_rel = 0.62;
    patch.cond_known = true;
    patch.wilted = true;
    // a stand before the first runs of the modules: no soil_rel (NaN), a condition equal to none of the codes
    VivariumPatchView unmeasured = patch;
    unmeasured.object_id = kOtherPatch;
    unmeasured.soil_rel = std::nan("");
    unmeasured.cond_known = false;
    const auto stands = plant_stands({patch, unmeasured});
    REQUIRE(stands.size() == 2u);
    // as the record carried them, narrowed to the float of a shader parameter and nothing else
    const PlantStand& wilted = stands.at(kPatch);
    CHECK(wilted.soil_known);
    CHECK(wilted.soil_rel == static_cast<float>(0.62));
    CHECK(wilted.condition_known);
    CHECK(wilted.wilted == 1.0f);
    CHECK(wilted.dead == 0.0f);
    // what the record did not carry stays unknown: the shader's default draws, never a mark made up
    const PlantStand& open = stands.at(kOtherPatch);
    CHECK_FALSE(open.soil_known);
    CHECK_FALSE(open.condition_known);
    CHECK(open.wilted == 0.0f);
    CHECK(open.dead == 0.0f);
    // a dead stand carries the dead mark alone; a mark on a condition equal to no code never reaches the shader
    VivariumPatchView dead = patch;
    dead.wilted = false;
    dead.dead = true;
    CHECK(plant_stands({dead}).at(kPatch).dead == 1.0f);
    CHECK(plant_stands({dead}).at(kPatch).wilted == 0.0f);
    dead.cond_known = false;
    CHECK(plant_stands({dead}).at(kPatch).dead == 0.0f);
    // a default stand is an unknown one - the ground of a place without a record
    const PlantStand none;
    CHECK_FALSE(none.soil_known);
    CHECK_FALSE(none.condition_known);
    CHECK(plant_stands({}).empty());
}

}  // namespace ase::adp::godot
