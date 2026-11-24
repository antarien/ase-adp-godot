/**
 * ASE ADAPTER TESTS - GODOT HOST CAP OF A FRUIT BODY
 *
 * @file        test_godot_host_cap.cpp
 * @design      DSGN_018
 * @brief       Pins the cap radius the cap port hands to the view for the death cap sown on place 1, and for no other patch
 * @description DSGN_018 ORGXFORM, usecase:101362 scale the organ templates: ase-plant carries the cap radius of the
 *              death cap (PLT_SPEC_CAP_RADIUS 0.05 m), ase-phytomer passes it on for the individual (PHY_CAP_RADIUS),
 *              ase-pl-flora files it on flora.cap.v1, and GodotHostResourceManager keeps it per patch (get_caps).
 *              The case sows amanita_phalloides on place 1 beside three patches of arosaruebe and waits until the
 *              port carries the patch of place 1: its radius is the one of the catalog, and a species without the
 *              key carries no record.
 *
 *              ONE GAME HOST PER RUN, in its own executable beside the others of the bundle.
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

#include "test_godot_host_support.hpp"

#include <ase/adp/godot/godot_host_resource_manager_class.hpp>

#include <cstdint>
#include <unordered_map>

namespace ase::adp::godot {

using test::AdapterEnvironment;
using test::advance_frames;
using test::environment;
using test::patch_at;

namespace {

constexpr uint32_t kCapPlace = 1u;          // the place the case sows the death cap on, as the client's start configuration does
constexpr double kDeathCapRadiusM = 0.05;   // PLT_SPEC_CAP_RADIUS of amanita_phalloides (modules/ase-plant/data/plant_species.json)
constexpr uint32_t kBoundFrames = 256u;     // frames the case waits for the cap record at most

// The death cap on place 1, the other three places keep the species of the shared configuration.
void sow_death_cap(VivariumStartConfig& config) {
    for (VivariumPatchSpec& patch : config.patches) {
        if (patch.place == kCapPlace) {
            patch.species = "amanita_phalloides";
        }
    }
}

}  // namespace

TEST_CASE("usecase:101362: the cap port carries the radius of the death cap for its patch and for no other") {
    AdapterEnvironment& env = environment(sow_death_cap);
    REQUIRE(env.booted == ase::kernel::HostStatusOk);
    GodotHostResourceManager& resources = env.resources;

    advance_frames(resources, 8u);
    const uint32_t capped = patch_at(resources, kCapPlace).object_id;
    for (uint32_t frame = 0u; frame < kBoundFrames && resources.get_caps().count(capped) == 0u; ++frame) {
        advance_frames(resources, 1u);
    }
    const std::unordered_map<uint32_t, double>& caps = resources.get_caps();
    const auto cap = caps.find(capped);
    REQUIRE(cap != caps.end());
    CHECK(cap->second == doctest::Approx(kDeathCapRadiusM));
    // arosaruebe carries no PLT_SPEC_CAP_RADIUS: its patches carry no record of the cap port
    for (uint32_t place = 0u; place < 4u; ++place) {
        if (place != kCapPlace) {
            CHECK(caps.count(patch_at(resources, place).object_id) == 0u);
        }
    }
}

}  // namespace ase::adp::godot
