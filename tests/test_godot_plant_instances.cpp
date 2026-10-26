/**
 * ASE ADAPTER TESTS - INSTANCE RECORDS OF THE PLANT WITHOUT GODOT
 *
 * @file        test_godot_plant_instances.cpp
 * @design      DSGN_066
 * @brief       Pins that the instance records reach their body and the side of the order split unchanged
 * @description PLAN_ASE_VIVARIUM_PHASE_03_ASSETS 03.11 step 2: rows_by_body sorts the records of an instance
 *              port by the body and the order their organ record names and hands the twelve values of each to
 *              the MultiMesh buffer as the port carried them (godot_plant_geometry.hpp).
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:101348 - hand organ instances to app
 * @actor       Plant Instance Perception Control (PLNTINST)
 * @created     2026-10-26
 * @modified    2026-10-26
 * @version     00.00.01.00000
 */

#include "test_godot_plant_records.hpp"

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_instances.hpp>
#include <ase/adp/godot/types.hpp>

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
    const auto split = rows_by_body(instances, organs, 2.0);
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
    const auto every = rows_by_body(instances, organs, GODOT_ORDER_EVERY);
    CHECK(every.at(kPatch).up_to.empty());
    REQUIRE(every.at(kPatch).above.size() == 3u * GODOT_INSTANCE_VALUES);
    check_carries(every.at(kPatch).above, 0u, instance(11u));
    check_carries(every.at(kPatch).above, 2u, instance(13u));

    // no records, no plants; records without organ records, no plants either
    CHECK(rows_by_body({}, organs, 2.0).empty());
    CHECK(rows_by_body(instances, {}, 2.0).empty());
}

}  // namespace ase::adp::godot
