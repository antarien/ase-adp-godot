/**
 * ASE ADAPTER TESTS - THE SCALE AND THE NAMES OF A FRUIT BODY WITHOUT GODOT
 *
 * @file        test_godot_cap_scale.cpp
 * @design      DSGN_018
 * @brief       Pins the scale that gives a drawn fruit body its cap radius, and the cap role and node of a species
 * @description DSGN_018 ORGXFORM, usecase:101362 scale the organ templates, the part of the cap drawing that carries
 *              no Godot type (godot_cap_scale.hpp): the radius over half the wider horizontal side of the drawn
 *              mesh, no scale for a mesh without extent or a radius that is not positive, and the role and child
 *              names of a species, read back from the role the stage names.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101362 - scale the organ templates
 * @actor       Organ Transform Shaper (ORGATRASHA)
 * @created     2026-11-24
 * @modified    2026-11-24
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include <doctest/doctest.h>

#include <ase/adp/godot/cap/godot_cap_scale.hpp>
#include <ase/adp/godot/cap/types.hpp>

#include <string>

namespace ase::adp::godot {

namespace {

constexpr float kDeathCapRadiusM = 0.05f;  // PLT_SPEC_CAP_RADIUS of amanita_phalloides (modules/ase-plant/data/plant_species.json)
constexpr float kUnitWidth = 1.0f;         // horizontal extent of the unit fruit body of the template port (GODOT_TMPL_FNGS)

}  // namespace

TEST_CASE("usecase:101362: the cap of the unit fruit body reaches the radius of the death cap") {
    // half the width of the unit body is 0.5 m: 0.05 m over it is a tenth
    CHECK(cap_scale(kDeathCapRadiusM, kUnitWidth, kUnitWidth) == doctest::Approx(0.1f));
}

TEST_CASE("usecase:101362: the wider horizontal side sets the reach") {
    // a mesh 0.2 m along x and 0.4 m along z reaches 0.2 m from its axis: 0.05 m over it is a quarter
    CHECK(cap_scale(kDeathCapRadiusM, 0.2f, 0.4f) == doctest::Approx(0.25f));
    CHECK(cap_scale(kDeathCapRadiusM, 0.4f, 0.2f) == doctest::Approx(0.25f));
}

TEST_CASE("usecase:101362: no extent or no positive radius gives no scale") {
    CHECK(cap_scale(kDeathCapRadiusM, 0.0f, 0.0f) == 0.0f);
    CHECK(cap_scale(0.0f, kUnitWidth, kUnitWidth) == 0.0f);
    CHECK(cap_scale(-kDeathCapRadiusM, kUnitWidth, kUnitWidth) == 0.0f);
}

TEST_CASE("usecase:101362: every species has its own cap role and child, read back from the role") {
    CHECK(cap_role_of("amanita_phalloides") == std::string(GODOT_ROLE_CAP) + "amanita_phalloides");
    CHECK(cap_species_of(cap_role_of("amanita_phalloides")) == "amanita_phalloides");
    CHECK(cap_species_of(cap_role_of("boletus")) == "boletus");
    CHECK(cap_node_of("amanita_phalloides") == std::string(GODOT_PLANT_CAP_KIND) + "amanita_phalloides");
    CHECK(cap_node_of("amanita_phalloides") != cap_node_of("boletus"));
}

TEST_CASE("usecase:101362: a role that is no cap role names no species") {
    CHECK(cap_species_of("bark").empty());
    CHECK(cap_species_of("_" + cap_role_of("boletus")).empty());  // the description key beside a role
    CHECK(cap_species_of(GODOT_ROLE_CAP).empty());               // the prefix alone names no species
}

}  // namespace ase::adp::godot
