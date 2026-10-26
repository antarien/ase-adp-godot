#pragma once

/**
 * ASE ADAPTER TESTS - ORGAN AND INSTANCE RECORDS OF THE PLANT TESTS
 *
 * @file        test_godot_plant_records.hpp
 * @design      DSGN_066
 * @brief       Builds organ and instance records as their ports lay them, and checks that a buffer carries one
 * @description Shared by test_godot_plant_instances.cpp and test_godot_plant_bark.cpp: an organ record with
 *              its body and the order of its axis, the instance record of that organ with twelve values no
 *              other organ of a test carries, and the check that a buffer of rows_by_body carries those twelve
 *              values unchanged.
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

#include <doctest/doctest.h>

#include <ase/adp/godot/godot_plant_geometry.hpp>
#include <ase/adp/godot/types.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ase::adp::godot::test_records {

inline constexpr uint32_t kPatch = 7u;       // object id of the patch the test plant stands on
inline constexpr uint32_t kOtherPatch = 8u;  // a patch the view does not place

/** An organ record as its port lays it: the organ, the patch its plant stands on, the order of its axis. */
inline VivariumOrganView organ(uint32_t object_id, uint32_t body, double order) {
    VivariumOrganView view;
    view.object_id = object_id;
    view.body = body;
    view.order = order;
    return view;
}

/** The instance record of that organ: twelve values no other organ of the test carries. */
inline VivariumInstanceView instance(uint32_t object_id) {
    VivariumInstanceView view;
    view.object_id = object_id;
    for (uint32_t v = 0u; v < GODOT_INSTANCE_VALUES; ++v) {
        view.rows[v] = static_cast<float>(object_id) + 0.01f * static_cast<float>(v);
    }
    return view;
}

/** Checks that `rows` carries the twelve values of `record` from record slot `slot` on, each exactly. */
inline void check_carries(const std::vector<float>& rows, std::size_t slot, const VivariumInstanceView& record) {
    REQUIRE(rows.size() >= (slot + 1u) * GODOT_INSTANCE_VALUES);
    for (uint32_t v = 0u; v < GODOT_INSTANCE_VALUES; ++v) {
        CHECK(rows[slot * GODOT_INSTANCE_VALUES + v] == record.rows[v]);
    }
}

}  // namespace ase::adp::godot::test_records
