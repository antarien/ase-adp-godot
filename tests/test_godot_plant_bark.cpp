/**
 * ASE ADAPTER TESTS - BARK OF THE INTERNODES WITHOUT GODOT
 *
 * @file        test_godot_plant_bark.cpp
 * @design      DSGN_024
 * @brief       Pins the bark share and the bark tiling every internode instance carries as custom data
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2: rows_by_body puts beside each instance record
 *              the bark share of its organ (white bark against stem-base bark) and the tiling of its section,
 *              which the bark shader mixes by (godot_plant_geometry.hpp, GODOT_BARK_SHADER_CODE).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:100395 - age the bark
 * @actor       Birkenborke (Betula pendula)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.01.00000
 */

#include "test_godot_plant_records.hpp"

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstdint>

namespace ase::adp::godot {

using test_records::check_carries;
using test_records::instance;
using test_records::kPatch;
using test_records::organ;

TEST_CASE("usecase:100395 age the bark: every instance carries the bark share of its organ as custom data, slot for slot") {
    VivariumOrganView stem = organ(11u, kPatch, 0.0);
    stem.bark = 0.75;  // the stem base, mostly stem-base bark
    VivariumOrganView band = organ(13u, kPatch, 0.0);
    band.bark = 0.25;  // the band towards the white bark
    const VivariumOrganView twig = organ(12u, kPatch, 3.0);  // a branch: white bark
    const auto split = rows_by_body({instance(11u), instance(12u), instance(13u)}, {stem, twig, band}, 2.0);
    const PlantBodyRows& plant = split.at(kPatch);

    // one custom slot per record, beside it in the same order: the share first, then the tiling - here none
    REQUIRE(plant.up_to_custom.size() == 2u * GODOT_INSTANCE_CUSTOM_VALUES);
    REQUIRE(plant.above_custom.size() == GODOT_INSTANCE_CUSTOM_VALUES);
    CHECK(plant.up_to_custom[0] == doctest::Approx(0.75));
    CHECK(plant.up_to_custom[GODOT_INSTANCE_CUSTOM_VALUES] == doctest::Approx(0.25));
    CHECK(plant.above_custom[0] == doctest::Approx(GODOT_BARK_WHITE));
    for (uint32_t v = 1u; v < GODOT_INSTANCE_CUSTOM_VALUES; ++v) {
        CHECK(plant.up_to_custom[v] == doctest::Approx(GODOT_TILE_NONE));
        CHECK(plant.up_to_custom[GODOT_INSTANCE_CUSTOM_VALUES + v] == doctest::Approx(GODOT_TILE_NONE));
        CHECK(plant.above_custom[v] == doctest::Approx(GODOT_TILE_NONE));
    }
    // the records themselves stay as the port carried them
    check_carries(plant.up_to, 0u, instance(11u));
    check_carries(plant.up_to, 1u, instance(13u));
    check_carries(plant.above, 0u, instance(12u));
}

TEST_CASE("03.11 step 2: every instance carries the bark tiling of its section after the bark share, unchanged") {
    VivariumOrganView stem = organ(11u, kPatch, 0.0);
    stem.bark = 0.5;
    stem.tile_around = 2.5;   // tiles of the white bark around the tube
    stem.tile_along = 0.75;   // tiles of the white bark along the tube
    stem.tile_base = 0.4;     // tiles of the stem-base bark per white tile
    const auto split = rows_by_body({instance(11u)}, {stem}, 2.0);
    const PlantBodyRows& plant = split.at(kPatch);

    REQUIRE(plant.up_to_custom.size() == GODOT_INSTANCE_CUSTOM_VALUES);
    CHECK(plant.up_to_custom[0] == doctest::Approx(0.5));
    CHECK(plant.up_to_custom[1] == doctest::Approx(2.5));
    CHECK(plant.up_to_custom[2] == doctest::Approx(0.75));
    CHECK(plant.up_to_custom[3] == doctest::Approx(0.4));
}

}  // namespace ase::adp::godot
