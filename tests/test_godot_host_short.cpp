/**
 * ASE ADAPTER TESTS - GODOT HOST UNREACHABLE GROWING STAGE
 *
 * @file        test_godot_host_short.cpp
 * @design      DSGN_024
 * @brief       Pins the growing days the forecast port hands to the view for a birch whose growth time ends before CRESCENDO
 * @description Register D2676: the founder operator of ase-genetics shifts every gene of the birch of place 0 (seed 42)
 *              by its mutation strength (CAUSA_ASE_GENETICS), and its vigor falls so far that the stem lacks growing
 *              days to the seedling bound when its growth time ends. ase-phytomer files them (PHY_SIZE_SHRT_D),
 *              ase-plant publishes them for the stand (PLT_GROW_SHRT_D), ase-pl-flora files them on the forecast
 *              record, and GodotHostResourceManager keeps them per patch (stage_short_d). The case pours the seed
 *              until its shoot stands and waits until the forecast port answers for CRESCENDO: an hour or a lack,
 *              never neither. For this birch the answer is the lack, and it carries no hour beside it.
 *
 *              ONE GAME HOST PER RUN, in its own executable beside the others of the bundle.
 *
 * @module      ase-adp-godot
 * @layer       5 (Adapter)
 * @category    process/validation/check
 * @usecase     usecase:6592 - time growth by age
 * @actor       VPS Basic Growth Time (VPSGROWTHTIME)
 * @created     2026-11-27
 * @modified    2026-11-27
 * @version     00.00.01.00001 [init]
 *
 * Copyright (c) Jan Ohlmann. All rights reserved.
 * Licensed to ANTARES SIMULACAO LTDA. Use is governed by LICENSE.md and PJV.md in this repository.
 * SPDX-License-Identifier: LicenseRef-ASE-LIC
 */

#include "test_godot_host_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager_class.hpp>
#include <ase/adp/godot/port/types.hpp>

#include <cmath>
#include <cstdint>

namespace ase::adp::godot {

using test::AdapterEnvironment;
using test::advance_frames;
using test::environment;
using test::patch_at;
using test::vivarium_config;

namespace {

constexpr uint32_t kBirchPlace = 0u;      // the place of the birch of seed 42 in the client's start configuration
constexpr uint32_t kSoilBoundFrames = 32u; // frames the case waits for the soil class of the cell at most
constexpr uint32_t kBoundFrames = 256u;   // frames the case waits for a shoot or a forecast at most

// The forecast port has answered for the growing stage: an hour or a lack.
bool answered(const GodotHostResourceManager& resources, const VivariumPatchView& patch) {
    const auto position = static_cast<uint8_t>(patch.stage + 1u);
    return resources.stage_due_h(patch, position) > 0.0 || std::isfinite(resources.stage_short_d(patch, position));
}

}  // namespace

TEST_CASE("D2676: the forecast port reports CRESCENDO unreachable for the birch of seed 42, with the growing days it lacks") {
    AdapterEnvironment& env = environment();
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;

    advance_frames(resources, 8u);
    const uint32_t cared = patch_at(resources, kBirchPlace).object_id;
    // The camera stands before the patch: only a body inside a detail band around the camera grows its organs.
    const VivariumPatchView& seen = patch_at(resources, kBirchPlace);
    REQUIRE(resources.view(seen.x, seen.y + 3.0, seen.z + 6.0, seen.x, seen.y, seen.z) == ase::kernel::HostStatusOk);
    // The seed sprouts once its soil holds the share its species needs; the case pours as a player would, bounded by
    // the pours that fill the capacity of the cell once (the same walk as test_godot_host_tree.cpp).
    REQUIRE(test::await_soil_class(resources, kSoilBoundFrames));
    const double amount_mm = vivarium_config(env.bundle).irrigate_amount_mm;
    const double capacity_mm = patch_at(resources, kBirchPlace).capacity_mm;
    for (double poured_mm = amount_mm;
         poured_mm < capacity_mm + amount_mm && resources.get_organs(GODOT_ORGAN_SEGMENT).empty();
         poured_mm += amount_mm) {
        REQUIRE(resources.irrigate(cared, amount_mm) == ase::kernel::HostStatusOk);
        for (uint32_t frame = 0u; frame < kBoundFrames && resources.get_organs(GODOT_ORGAN_SEGMENT).empty(); ++frame) {
            advance_frames(resources, 1u);
        }
    }
    REQUIRE(!resources.get_organs(GODOT_ORGAN_SEGMENT).empty());

    for (uint32_t frame = 0u; frame < kBoundFrames && !answered(resources, patch_at(resources, kBirchPlace)); ++frame) {
        advance_frames(resources, 1u);
    }
    const VivariumPatchView& birch = patch_at(resources, kBirchPlace);
    REQUIRE(birch.sprout);
    const auto grow = static_cast<uint8_t>(birch.stage + 1u);
    const double due_h = resources.stage_due_h(birch, grow);
    const double short_d = resources.stage_short_d(birch, grow);
    CAPTURE(due_h);
    CAPTURE(short_d);
    // The port answers, and for this individual the answer is the lack: a positive count of growing days, no hour.
    REQUIRE(std::isfinite(short_d));
    CHECK(short_d > 0.0);
    CHECK(!(due_h > 0.0));
    // The mature stage carries no lack: ase-plant forecasts it from the age, which always grows.
    CHECK(!std::isfinite(resources.stage_short_d(birch, static_cast<uint8_t>(grow + 1u))));
}

}  // namespace ase::adp::godot
